#ifndef WIN32
#include <netinet/tcp.h>
#endif

#include <string.h>

#include <lualib.h>

#include "library.h"
#include "proto.h"
#include "script.h"

static void load_lib(lua_State* lua, const char* name, lua_CFunction func)
{
    luaL_requiref(lua, name, func, 1);
    lua_pop(lua, 1);
}

static int state_get(lua_State* lua)
{
    size_t len;
    const char* str = luaL_checklstring(lua, -1, &len);

    if (strcmp(str, "msg_ident") == 0)
        lua_pushinteger(lua, this.msg_ident);
    else if (strcmp(str, "msg_ttl") == 0)
        lua_pushinteger(lua, this.msg_ttl);
    else if (strcmp(str, "localip") == 0)
        lua_pushinteger(lua, this.localip);
    else if (strcmp(str, "netmask") == 0)
        lua_pushinteger(lua, this.netmask);
    else if (strcmp(str, "log_level") == 0)
        lua_pushinteger(lua, this.log_level);
    else if (strcmp(str, "little_endian") == 0)
        lua_pushinteger(lua, this.little_endian);
    else if (strcmp(str, "internal_mtu") == 0)
        lua_pushinteger(lua, this.internal_mtu);
    else if (strcmp(str, "use_udp") == 0)
        lua_pushinteger(lua, this.use_udp);
    else if (strcmp(str, "aes_key") == 0)
        lua_pushlstring(lua, (const char*)this.aes_key, sizeof(this.aes_key));
    else if (strcmp(str, "aes_key_len") == 0)
        lua_pushinteger(lua, this.aes_key_len);
    else if (strcmp(str, "aes_iv") == 0)
        lua_pushlstring(lua, (const char*)this.aes_iv, sizeof(this.aes_iv));
    else if (strcmp(str, "des_key") == 0)
        lua_pushlstring(lua, (const char*)this.des_key, sizeof(this.des_key));
    else if (strcmp(str, "des_key_len") == 0)
        lua_pushinteger(lua, this.des_key_len);
    else if (strcmp(str, "des_iv") == 0)
        lua_pushlstring(lua, (const char*)this.des_iv, sizeof(this.des_iv));
    else if (strcmp(str, "compress") == 0)
        lua_pushinteger(lua, this.compress);
    else if (strcmp(str, "encrypt") == 0)
        lua_pushinteger(lua, this.encrypt);
    else
        lua_pushnil(lua);
    return 1;
}

static int state_set_and_check(lua_State* lua, const char* key, int input_type)
{
    int rc = 0;
    if (strcmp(key, "msg_ident") == 0)
    {
        rc = input_type == LUA_TNUMBER;
        if (!rc) goto end;
        this.msg_ident = lua_tonumber(lua, 3);
    }
    else if (strcmp(key, "msg_ttl") == 0)
    {
        rc = input_type == LUA_TNUMBER;
        if (!rc) goto end;
        this.msg_ttl = lua_tonumber(lua, 3);
    }
    else if (strcmp(key, "localip") == 0)
    {
        rc = input_type == LUA_TNUMBER;
        if (!rc) goto end;
        this.localip = lua_tonumber(lua, 3);
    }
    else if (strcmp(key, "netmask") == 0)
    {
        rc = input_type == LUA_TNUMBER;
        if (!rc) goto end;
        this.netmask = lua_tonumber(lua, 3);
    }
    else if (strcmp(key, "log_level") == 0)
    {
        rc = input_type == LUA_TNUMBER;
        if (!rc) goto end;
        this.log_level = lua_tonumber(lua, 3);
    }
    else if (strcmp(key, "internal_mtu") == 0)
    {
        rc = input_type == LUA_TNUMBER;
        if (!rc) goto end;
        this.internal_mtu = lua_tonumber(lua, 3);
        this.max_length   = ROUND_UP(this.internal_mtu - sizeof(msg_t) - sizeof(struct iphdr) - (this.use_udp ? sizeof(struct udphdr) : sizeof(struct tcphdr)), 8);
    }
    else if (strcmp(key, "use_udp") == 0)
    {
        rc = input_type == LUA_TNUMBER || input_type == LUA_TBOOLEAN;
        if (!rc) goto end;
        this.use_udp = lua_tonumber(lua, 3);
    }
    else if (strcmp(key, "aes_key") == 0)
    {
        size_t len;
        const char* str;
        rc = input_type == LUA_TSTRING;
        if (!rc) goto end;
        str = luaL_checklstring(lua, 3, &len);
        memcpy(this.aes_key, str, MIN(sizeof(this.aes_key), len));
    }
    else if (strcmp(key, "aes_key_len") == 0)
    {
        rc = input_type == LUA_TNUMBER;
        if (!rc) goto end;
        this.aes_key_len = lua_tonumber(lua, 3);
    }
    else if (strcmp(key, "aes_iv") == 0)
    {
        size_t len;
        const char* str;
        rc = input_type == LUA_TSTRING;
        if (!rc) goto end;
        str = luaL_checklstring(lua, 3, &len);
        memcpy(this.aes_iv, str, MIN(sizeof(this.aes_iv), len));
    }
    else if (strcmp(key, "des_key") == 0)
    {
        size_t len;
        const char* str;
        rc = input_type == LUA_TSTRING;
        if (!rc) goto end;
        str = luaL_checklstring(lua, 3, &len);
        memcpy(this.des_key, str, MIN(sizeof(this.des_key), len));
    }
    else if (strcmp(key, "des_key_len") == 0)
    {
        rc = input_type == LUA_TNUMBER;
        if (!rc) goto end;
        this.des_key_len = lua_tonumber(lua, 3);
    }
    else if (strcmp(key, "des_iv") == 0)
    {
        size_t len;
        const char* str;
        rc = input_type == LUA_TSTRING;
        str = luaL_checklstring(lua, 3, &len);
        memcpy(this.des_iv, str, MIN(sizeof(this.des_iv), len));
    }
    else if (strcmp(key, "compress") == 0)
    {
        rc = input_type == LUA_TNUMBER;
        if (!rc) goto end;
        this.compress = lua_tonumber(lua, 3);
    }
    else if (strcmp(key, "encrypt") == 0)
    {
        rc = input_type == LUA_TNUMBER;
        if (!rc) goto end;
        this.encrypt = lua_tonumber(lua, 3);
    }
end:
    return rc;
}

static int state_set(lua_State* lua)
{
    size_t len;
    const char* str = luaL_checklstring(lua, 2, &len);
    int type = lua_type(lua, 3);

    if (!state_set_and_check(lua, str, type))
    {
        const char* msg = lua_pushfstring(lua, "the param [%s] is not support, or type [%s] is not supported", str, lua_typename(lua, type));
        return luaL_argerror(lua, 1, msg);
    }
    return 0;
}

static int _syslog(lua_State* lua)
{
    lua_Number level = luaL_checknumber(lua, 1);
    const char* str = luaL_checkstring(lua, 2);
    SYSLOG(level, str, "");
    return 0;
}

static void get_qtun_obj(lua_State* lua)
{
    lua_getglobal(lua, "qtun");
    if (lua_isnil(lua, -1))
    {
        lua_pop(lua, 1);
        lua_newtable(lua);
        lua_setglobal(lua, "qtun");
        lua_getglobal(lua, "qtun");
    }
}

static void push_log_level(lua_State* lua, const char* name, lua_Number value)
{
    lua_pushstring(lua, name);
    lua_pushinteger(lua, value);
    lua_settable(lua, -3);
}

static void init_qtun_state(lua_State* lua)
{
    get_qtun_obj(lua);
    lua_pushstring(lua, "state");
    lua_newtable(lua);
    lua_settable(lua, -3);
    lua_pushstring(lua, "state");
    lua_gettable(lua, -2);

    lua_newtable(lua);
    lua_pushstring(lua, "__index");
    lua_pushcfunction(lua, state_get);
    lua_settable(lua, -3);
    lua_pushstring(lua, "__newindex");
    lua_pushcfunction(lua, state_set);
    lua_settable(lua, -3);
    lua_setmetatable(lua, -2);

    lua_pop(lua, 1);
    {
        int new_log = 0;
        lua_pushstring(lua, "log");
        lua_gettable(lua, -2);
        if (lua_isnil(lua, -1))
        {
            lua_pop(lua, 1);
            lua_pushstring(lua, "log");
            lua_newtable(lua);
            new_log = 1;
        }
        push_log_level(lua, "LOG_EMERG"  , 0);
        push_log_level(lua, "LOG_ALERT"  , 1);
        push_log_level(lua, "LOG_CRIT"   , 2);
        push_log_level(lua, "LOG_ERR"    , 3);
        push_log_level(lua, "LOG_WARNING", 4);
        push_log_level(lua, "LOG_NOTICE" , 5);
        push_log_level(lua, "LOG_INFO"   , 6);
        push_log_level(lua, "LOG_DEBUG"  , 7);
        if (new_log)
            lua_settable(lua, -3);
    }
    lua_pop(lua, 1);
}

int script_global_init(lua_State* lua)
{
    load_lib(lua, "_G", luaopen_base);
    load_lib(lua, LUA_TABLIBNAME, luaopen_table);
    load_lib(lua, LUA_STRLIBNAME, luaopen_string);
    load_lib(lua, LUA_IOLIBNAME, luaopen_io);
    
    lua_pushcfunction(lua, _syslog);
    lua_setglobal(lua, "_syslog");
    
    init_qtun_state(lua);
    return 1;
}

int script_load_config(lua_State* lua, const char* file_path)
{
    return 1;
}

