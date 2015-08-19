/** @cover.cpp
*	控制器CGI
*	@author 
*	@version 1.0
*	@date	2014-11-04
*/

#include "comm_fun.h"
#include "Servlet.h"

namespace cgi
{
	class cover : public CServlet
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

		void handleRequest(CGIRequest& request)
		{		

			int iRet = 0;

			string strK = request.getTrimQueryVal("k");
    		request["k"] = strK;
    		request["uid"] = request.getTrimQueryVal("uid");
    		string strCid = request.getTrimQueryVal("cid");
    		request["cid"] = strCid;
			request["title"] = request.getTrimQueryVal("title");
			
			if ( "show" == strK )
			{
				//查看专辑内容
				int iDbState = 0;
				iRet = getCoverBaseInfo(request,iDbState);
				if (iRet != 0)
				{
					ISD_LOGGER_ERROR("get base info error,iRet = %d,cid = %s.",iRet,strCid.c_str());
				}

				if (100 == iDbState)
				{
					int iList = atoi(request.getTrimQueryVal("list").c_str());
					if (iList != 1)
					{
						request.redirect("/error.html");
					}
				}

			}
			else if ( "add" == strK )
			{
				
			}
			else if ( "sids" == strK )
			{
				//获取单图列表
				request["cate"] = 0;
			}
			else
			{

			}
			return;
		}
		
		private:

			int getCoverBaseInfo(CGIRequest& request,int &iDbState)
			{
				int iRet = 0;
				string strCid = request.getTrimQueryVal("cid");
				if ( strCid.empty() )
				{
					request["k"] = "search";
					return 10;
				}

				char szSql[MAX_SQL_LENGTH] = {0};
				snprintf(szSql,sizeof(szSql)-1,"SELECT c_title,c_date,c_img_url,c_desc,c_state FROM d_config_system.t_cover where c_cid='%s';",_cMysqlUtil.EscapeString2(strCid).c_str());
				string strTitle,strDate,strImgUrl,strDesc;
				
				MYSQL_EXEC_GET_BEGIN(_cMysqlUtil, szSql, iRet)
				{
					unsigned int uiPos = 0;
					GET_STR_COLUMN_VALUE_SCROLL(strTitle, uiPos)
					GET_STR_COLUMN_VALUE_SCROLL(strDate, uiPos)
					GET_STR_COLUMN_VALUE_SCROLL(strImgUrl, uiPos)
					GET_STR_COLUMN_VALUE_SCROLL(strDesc, uiPos)
					GET_INT_COLUMN_VALUE_SCROLL(iDbState, uiPos)
				}MYSQL_EXEC_GET_END()
				if(iRet != 0)
				{
					ISD_LOGGER_ERROR("exec sql error: szSql = %s, iRet = %d.", szSql, iRet);
					return 10;
				}

				request["title"] = strTitle;
				request["date"] = strDate;
				request["img"] = strImgUrl;
				request["desc"] = strDesc;
				
				request["state"] = iDbState;
				request["state_desc"] = getStateName(iDbState);
						

				return 0;
			}

			string getStateName(int iState)
			{
				string strName;
				switch (iState)
				{
					case 0:
					{
						strName = "初始状态";
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
	static cgi::cover cServlet;
	cServlet.run();
	return 0;
}
