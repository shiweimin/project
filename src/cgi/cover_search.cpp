/** @file cover_search.cpp
 *  更新专辑状态cgi 通用
 *  @author arikshi
 *  @version 1.0.0
 *  @date 2015-02-07
*/

#include "comm_fun.h"
#include "Servlet.h"

#define C_ID_LENGTH	6

namespace cgi
{
	class cover_search : public CAjaxServlet
	{
		public:

			void init()
			{
				int iRet = 0;

				string strDBIP = g_configger->getStr("db_ip", "localhost");
				string strDBName = g_configger->getStr("db_name", "d_config_system");
				string strUserName = g_configger->getStr("db_user_name", "root");
				string strUserPwd = g_configger->getStr("db_pwd", "sega1234");
				int iPort = g_configger->getInt("db_port",3306);
				int iTimeOut = g_configger->getInt("db_timeout",2);
				ISD_LOGGER_TRACE("DBIP = %s, DBName = %s, UserName = %s, UserPwd = %s, Port = %d, timeout = %d.", \
					strDBIP.c_str(), strDBName.c_str(), strUserName.c_str(), strUserPwd.c_str(),iPort,iTimeOut);
				iRet = _cMysqlUtil.Open(strDBIP.c_str(), strUserName.c_str(), strUserPwd.c_str(), strDBName.c_str(),iTimeOut,iPort);
				if(iRet < 0)
				{
					ISD_LOGGER_ERROR("_cMysqlUtil Open Error: iRet = %d.", iRet);
					exit(0);
				}
				_cMysqlUtil.SetCharacter("utf8");

				ISD_LOGGER_TRACE("init sucessful!");
			}

			void handleAjaxRequest(CGIRequest& request,Json::Value& root)
			{
				ISD_LOGGER_TRACE("cgi begin.");
				
				//判断用户的合法性
				string strUid = request.getTrimQueryVal("uid");

				string strErrorMsg;
				int iRet = argsIsIllegal(strUid,strErrorMsg);
				if ( iRet != 0 )
				{
					root["ret"] = 10;
					root["msg"] = strErrorMsg;
					return;
				}
	
				string strWhere;
				bool bFlag = false;
				string strCid = request.getTrimQueryVal("cid",bFlag);
				if (bFlag && !strCid.empty())
				{
					strWhere = "c_cid='" + _cMysqlUtil.EscapeString2(strCid) + "'";
				}
				else
				{
					string strTitle = request.getTrimQueryVal("title",bFlag);
					if (bFlag && !strTitle.empty())
					{
						strWhere = "c_title like '%" + _cMysqlUtil.EscapeString2(strTitle) + "%'";
					}
				}
				
				int iState = atoi(request.getTrimQueryVal("state",bFlag).c_str());
				if (bFlag && iState != -1)
				{
					if (strWhere.empty())
					{
						strWhere = "c_state=" + intToString(iState);
					}
					else
					{
						strWhere += " and c_state=" + intToString(iState);
					}
				}
				
				if (!strWhere.empty())
				{
					strWhere = "and " + strWhere;
				}
				
				int iStartPage = 0;
				int iPage = atoi(request.getQueryVal("p").c_str());					//当前页
				if ( iPage > 0 )
				{
					iStartPage = iPage - 1;
				}
				int iRecordNumberOfPage = atoi(request.getQueryVal("psz").c_str());	//每页结果数
				if ( iRecordNumberOfPage <= 0 || iRecordNumberOfPage > 20)
				{
					iRecordNumberOfPage = 20;
				}
				
				char szSql[MAX_SQL_LENGTH] = {0};
				snprintf(szSql,sizeof(szSql)-1,"select c_cid,c_title,c_date,c_mtime,c_state,c_longpic_num,c_normalpic_num from d_config_system.t_cover where c_uid='%s' %s order by c_mtime desc limit %d,%d;",_cMysqlUtil.EscapeString2(strUid).c_str(),strWhere.c_str(),iStartPage*iRecordNumberOfPage,iRecordNumberOfPage);

				int iIndex = iStartPage*iRecordNumberOfPage;
				MYSQL_EXEC_GET_BEGIN(_cMysqlUtil, szSql, iRet)
				{
					string strCid,strTitle,strDate,strMtime;
					int iState = 0,iLongPicNum = 0,iNormalPicNum = 0;
					unsigned int uiPos = 0;
					GET_STR_COLUMN_VALUE_SCROLL(strCid, uiPos)
					GET_STR_COLUMN_VALUE_SCROLL(strTitle, uiPos)
					GET_STR_COLUMN_VALUE_SCROLL(strDate, uiPos)
					GET_STR_COLUMN_VALUE_SCROLL(strMtime, uiPos)
					GET_INT_COLUMN_VALUE_SCROLL(iState, uiPos)
					GET_INT_COLUMN_VALUE_SCROLL(iLongPicNum, uiPos)
					GET_INT_COLUMN_VALUE_SCROLL(iNormalPicNum, uiPos)
					
					Json::Value json;

					json["no"] = ++iIndex;
					json["cid"] = strCid;
					json["ti"] = strTitle;
					json["d"] = strDate;
					json["m"] = strMtime;
					json["sn"] = getStateName(iState);
					json["ln"] = iLongPicNum;
					json["rn"] = iNormalPicNum;
					
					root["node"].append(json);
				}MYSQL_EXEC_GET_END()
				if(iRet != 0)
				{
					ISD_LOGGER_ERROR("exec sql error: szSql = %s, iRet = %d.", szSql, iRet);
					root["ret"] = 50;
					root["msg"] = "服务器不通，请稍后再试";
					return;
				}
				
				int iAllNum = 0;
				iRet = getAllNum(strUid,strWhere,iAllNum);
				if(iRet != 0)
				{
					ISD_LOGGER_ERROR("获取总数失败,uid = %s,where = %s,iRet = %d.", strUid.c_str(),strWhere.c_str(), iRet);
					root["ret"] = 60;
					root["msg"] = "服务器不通，请稍后再试";
					return;
				}
				
				int iTotalPage = iAllNum/iRecordNumberOfPage;
				if (iAllNum > iRecordNumberOfPage)
				{
					iTotalPage += iAllNum%iRecordNumberOfPage > 0 ? 1 : 0;
				}
				root["p"] = iPage==0 ? 1 : iPage;	//curent page number
				root["tp"] = iTotalPage;			//total page number
				root["an"] = iAllNum;				//all number

				root["ret"] = 0;
				root["msg"] = "suc.";

				ISD_LOGGER_TRACE("cgi end.");
			}

		private:

			int argsIsIllegal(const string &strUid,string &strMsg)
			{
				int iRet = 0;
				char szSql[MAX_SQL_LENGTH] = {0};
				snprintf(szSql,sizeof(szSql)-1,"SELECT c_pwd,c_available_flag FROM d_config_system.t_user_info where c_uid='%s';",_cMysqlUtil.EscapeString2(strUid).c_str());
				string strPassword;
				int iAvilableFlag = 0;//0:yes 1:lock 2:delete
				MYSQL_EXEC_GET_BEGIN(_cMysqlUtil, szSql, iRet)
				{
					unsigned int uiPos = 0;
					GET_STR_COLUMN_VALUE_SCROLL(strPassword, uiPos)
					GET_INT_COLUMN_VALUE_SCROLL(iAvilableFlag, uiPos)
				}MYSQL_EXEC_GET_END()
				if(iRet != 0)
				{
					ISD_LOGGER_ERROR("exec sql error: szSql = %s, iRet = %d.", szSql, iRet);
					return 10;
				}

				if (strPassword.empty())
				{
					strMsg = "用户不存在.";
					return -1;
				}

				if (iAvilableFlag != 0)
				{
					strMsg = "用户账号不可用.";
					return -2;
				}
				
				return 0;
			}
			
			int getAllNum(const string &strUid,const string &strWhere,int &iAllNum)
			{
				int iRet = 0;
				char szSql[MAX_SQL_LENGTH] = {0};
				snprintf(szSql,sizeof(szSql)-1,"select count(*) from d_config_system.t_cover where c_uid='%s' %s;",_cMysqlUtil.EscapeString2(strUid).c_str(),strWhere.c_str());
				
				MYSQL_EXEC_GET_BEGIN(_cMysqlUtil, szSql, iRet)
				{
					GET_INT_COLUMN_VALUE(iAllNum, 0)
				}MYSQL_EXEC_GET_END()
				if(iRet != 0)
				{
					ISD_LOGGER_ERROR("exec sql error: szSql = %s, iRet = %d.", szSql, iRet);
				}
				
				return iRet;
			}
			
			string intToString(const int &i)
			{
				char szChar[16] = {0};
				snprintf(szChar,sizeof(szChar)-1,"%d",i);
				return szChar;
			}
			
			string getStateName(const int &iState)
			{
				string strName;
				switch(iState)
				{
					case 0:
					{
						strName = "初始";
						break;
					}
					case 4:
					{
						strName = "已上架";
						break;
					}
					case 8:
					{
						strName = "已下架";
						break;
					}
					case 100:
					{
						strName = "已删除";
						break;
					}
					default:
					{
						strName = "未知";
						break;
					}
				}
				
				return strName;
			}

		private:
			CMysqlUtil _cMysqlUtil;	
	};
}

int main()
{
	static cgi::cover_search cServlet;
	cServlet.run();
	return 0;
}

