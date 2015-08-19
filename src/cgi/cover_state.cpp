/** @file cover_state.cpp
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
	class cover_state : public CAjaxServlet
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
				
				string strCid = request.getTrimQueryVal("cid");			
				int iState = atoi(request.getTrimQueryVal("state").c_str());
				
				string strErrorMsg;
				int iRet = argsIsIllegal(strUid,strCid,iState,strErrorMsg);
				if ( iRet != 0 )
				{
					root["ret"] = 10;
					root["msg"] = strErrorMsg;
					return;
				}
				
				char szSql[MAX_SQL_LENGTH] = {0};
				snprintf(szSql,sizeof(szSql)-1,"update d_config_system.t_cover set c_state=%d where c_cid='%s';",iState,_cMysqlUtil.EscapeString2(strCid).c_str());
				
				MYSQL_EXEC_BEGIN(_cMysqlUtil, szSql, iRet)
				{
				}MYSQL_EXEC_END()
				if(iRet != 0)
				{
					ISD_LOGGER_ERROR("exec sql error: szSql = %s, iRet = %d.", szSql, iRet);
					root["ret"] = 50;
					root["msg"] = "服务器不通，请稍后再试";
					return;
				}

				root["ret"] = 0;
				root["msg"] = "suc.";

				ISD_LOGGER_TRACE("cgi end.");
			}

		private:

			int argsIsIllegal(const string &strUid,const string &strCid,const int &iType,string &strMsg)
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
				
				if (strCid.length() != C_ID_LENGTH)
				{
					strMsg = "专辑ID不合法.";
					return -3;
				}
				
				if (iType != 4 && iType != 8 && iType != 100)
				{
					strMsg = "state不合法.";
					return -4;
				}

				return 0;
			}

		private:
			CMysqlUtil _cMysqlUtil;	
	};
}

int main()
{
	static cgi::cover_state cServlet;
	cServlet.run();
	return 0;
}

