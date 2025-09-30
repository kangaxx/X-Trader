#include "redis_client.h"
#include <iostream>
#include <cstring>

RedisClient& RedisClient::getInstance() {
    static RedisClient instance;
    return instance;
}

#ifndef _WIN32

RedisClient::RedisClient() : m_context(nullptr), m_connected(false) {}

RedisClient::~RedisClient() {
    disconnect();
}

bool RedisClient::connect(const std::string& host, int port, int timeout) {
    if (m_connected) {
        disconnect();
    }
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    m_context = redisConnectWithTimeout(host.c_str(), port, tv);
    if (m_context == nullptr || m_context->err) {
        if (m_context) {
            std::cerr << "Redis连接失败: " << m_context->errstr << std::endl;
            redisFree(m_context);
            m_context = nullptr;
        }
        else {
            std::cerr << "Redis连接失败: 内存分配错误" << std::endl;
        }
        m_connected = false;
        return false;
    }
    m_connected = true;
    std::cout << "Redis连接成功" << std::endl;
    return true;
}

void RedisClient::disconnect() {
    if (m_context) {
        redisFree(m_context);
        m_context = nullptr;
    }
    m_connected = false;
    std::cout << "Redis连接已断开" << std::endl;
}

bool RedisClient::isConnected() const {
    return m_connected;
}

bool RedisClient::set(const std::string& key, const std::string& value) {
    if (!m_connected) {
        std::cerr << "Redis未连接" << std::endl;
        return false;
    }
    redisReply* reply = (redisReply*)redisCommand(m_context, "SET %s %s", key.c_str(), value.c_str());
    if (!reply) {
        std::cerr << "SET命令执行失败: " << m_context->errstr << std::endl;
        return false;
    }
    bool success = (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0);
    freeReplyObject(reply);
    return success;
}

std::string RedisClient::get(const std::string& key) {
    if (!m_connected) {
        std::cerr << "Redis未连接" << std::endl;
        return "";
    }
    redisReply* reply = (redisReply*)redisCommand(m_context, "GET %s", key.c_str());
    if (!reply) {
        std::cerr << "GET命令执行失败: " << m_context->errstr << std::endl;
        return "";
    }
    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = reply->str;
    }
    else if (reply->type == REDIS_REPLY_NIL) {
        result = "";
    }
    else {
        std::cerr << "GET命令返回意外类型: " << reply->type << std::endl;
    }
    freeReplyObject(reply);
    return result;
}

long long RedisClient::del(const std::string& key) {
    if (!m_connected) {
        std::cerr << "Redis未连接" << std::endl;
        return -1;
    }
    redisReply* reply = (redisReply*)redisCommand(m_context, "DEL %s", key.c_str());
    if (!reply) {
        std::cerr << "DEL命令执行失败: " << m_context->errstr << std::endl;
        return -1;
    }
    long long deleted = reply->integer;
    freeReplyObject(reply);
    return deleted;
}

bool RedisClient::exists(const std::string& key) {
    if (!m_connected) {
        std::cerr << "Redis未连接" << std::endl;
        return false;
    }
    redisReply* reply = (redisReply*)redisCommand(m_context, "EXISTS %s", key.c_str());
    if (!reply) {
        std::cerr << "EXISTS命令执行失败: " << m_context->errstr << std::endl;
        return false;
    }
    bool exists = (reply->integer == 1);
    freeReplyObject(reply);
    return exists;
}

bool RedisClient::expire(const std::string& key, int seconds) {
    if (!m_connected) {
        std::cerr << "Redis未连接" << std::endl;
        return false;
    }
    redisReply* reply = (redisReply*)redisCommand(m_context, "EXPIRE %s %d", key.c_str(), seconds);
    if (!reply) {
        std::cerr << "EXPIRE命令执行失败: " << m_context->errstr << std::endl;
        return false;
    }
    bool success = (reply->integer == 1);
    freeReplyObject(reply);
    return success;
}

long long RedisClient::lpush(const std::string& listKey, const std::string& value) {
    if (!m_connected) {
        std::cerr << "Redis未连接" << std::endl;
        return -1;
    }
    redisReply* reply = (redisReply*)redisCommand(m_context, "LPUSH %s %s", listKey.c_str(), value.c_str());
    if (!reply) {
        std::cerr << "LPUSH命令执行失败: " << m_context->errstr << std::endl;
        return -1;
    }
    long long length = reply->integer;
    freeReplyObject(reply);
    return length;
}

std::string RedisClient::rpop(const std::string& listKey) {
    if (!m_connected) {
        std::cerr << "Redis未连接" << std::endl;
        return "";
    }
    redisReply* reply = (redisReply*)redisCommand(m_context, "RPOP %s", listKey.c_str());
    if (!reply) {
        std::cerr << "RPOP命令执行失败: " << m_context->errstr << std::endl;
        return "";
    }
    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = reply->str;
    }
    else if (reply->type == REDIS_REPLY_NIL) {
        result = "";
    }
    else {
        std::cerr << "RPOP命令返回意外类型: " << reply->type << std::endl;
    }
    freeReplyObject(reply);
    return result;
}

bool RedisClient::ltrim(const std::string& listKey, int start, int stop) {
    if (!m_connected) return false;
    redisReply* reply = (redisReply*)redisCommand(m_context, "LTRIM %s %d %d", listKey.c_str(), start, stop);
    if (!reply) return false;
    bool success = (reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
    freeReplyObject(reply);
    return success;
}

std::vector<std::string> RedisClient::lrange(const std::string& listKey, int start, int stop) {
    std::vector<std::string> result;
    if (!m_connected) {
        std::cerr << "Redis未连接" << std::endl;
        return result;
    }
    redisReply* reply = (redisReply*)redisCommand(m_context, "LRANGE %s %d %d", listKey.c_str(), start, stop);
    if (!reply) {
        std::cerr << "LRANGE命令执行失败: " << m_context->errstr << std::endl;
        return result;
    }
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; ++i) {
            redisReply* elem = reply->element[i];
            if (elem->type == REDIS_REPLY_STRING) {
                result.emplace_back(elem->str, elem->len);
            }
        }
    }
    else {
        std::cerr << "LRANGE命令返回意外类型: " << reply->type << std::endl;
    }
    freeReplyObject(reply);
    return result;
}

#else // _WIN32

RedisClient::RedisClient() {}
RedisClient::~RedisClient() {}

bool RedisClient::connect(const std::string&, int, int) { return false; }
void RedisClient::disconnect() {}
bool RedisClient::isConnected() const { return false; }
bool RedisClient::set(const std::string&, const std::string&) { return false; }
std::string RedisClient::get(const std::string&) { return ""; }
long long RedisClient::del(const std::string&) { return 0; }
bool RedisClient::exists(const std::string&) { return false; }
bool RedisClient::expire(const std::string&, int) { return false; }
long long RedisClient::lpush(const std::string&, const std::string&) { return 0; }
std::string RedisClient::rpop(const std::string&) { return ""; }
bool RedisClient::ltrim(const std::string&, int, int) { return false; }
std::vector<std::string> RedisClient::lrange(const std::string&, int, int) { return {}; }

#endif