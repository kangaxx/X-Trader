#include "redis_client.h"
#include <iostream>

RedisClient::RedisClient() : m_context(nullptr), m_connected(false) {}

RedisClient::~RedisClient() {
    disconnect();
}

RedisClient& RedisClient::getInstance() {
    static RedisClient instance;
    return instance;
}

bool RedisClient::connect(const std::string& host, int port, int timeout) {
    disconnect();
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    m_context = redisConnectWithTimeout(host.c_str(), port, tv);
    if (m_context == nullptr || m_context->err) {
        if (m_context) {
            std::cerr << "Redis connect error: " << m_context->errstr << std::endl;
            redisFree(m_context);
            m_context = nullptr;
        }
        else {
            std::cerr << "Redis connect error: can't allocate redis context" << std::endl;
        }
        m_connected = false;
        return false;
    }
    m_connected = true;
    return true;
}

void RedisClient::disconnect() {
    if (m_context) {
        redisFree(m_context);
        m_context = nullptr;
    }
    m_connected = false;
}

bool RedisClient::isConnected() const {
    return m_connected;
}

bool RedisClient::set(const std::string& key, const std::string& value) {
    if (!m_connected) return false;
    redisReply* reply = (redisReply*)redisCommand(m_context, "SET %s %s", key.c_str(), value.c_str());
    bool success = reply && reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK";
    if (reply) freeReplyObject(reply);
    return success;
}

std::string RedisClient::get(const std::string& key) {
    if (!m_connected) return "";
    redisReply* reply = (redisReply*)redisCommand(m_context, "GET %s", key.c_str());
    std::string value;
    if (reply && reply->type == REDIS_REPLY_STRING) {
        value = reply->str;
    }
    if (reply) freeReplyObject(reply);
    return value;
}

long long RedisClient::del(const std::string& key) {
    if (!m_connected) return 0;
    redisReply* reply = (redisReply*)redisCommand(m_context, "DEL %s", key.c_str());
    long long ret = 0;
    if (reply && reply->type == REDIS_REPLY_INTEGER) {
        ret = reply->integer;
    }
    if (reply) freeReplyObject(reply);
    return ret;
}

bool RedisClient::exists(const std::string& key) {
    if (!m_connected) return false;
    redisReply* reply = (redisReply*)redisCommand(m_context, "EXISTS %s", key.c_str());
    bool ret = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
    if (reply) freeReplyObject(reply);
    return ret;
}

bool RedisClient::expire(const std::string& key, int seconds) {
    if (!m_connected) return false;
    redisReply* reply = (redisReply*)redisCommand(m_context, "EXPIRE %s %d", key.c_str(), seconds);
    bool ret = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
    if (reply) freeReplyObject(reply);
    return ret;
}

long long RedisClient::lpush(const std::string& listKey, const std::string& value) {
    if (!m_connected) return 0;
    redisReply* reply = (redisReply*)redisCommand(m_context, "LPUSH %s %s", listKey.c_str(), value.c_str());
    long long ret = 0;
    if (reply && reply->type == REDIS_REPLY_INTEGER) {
        ret = reply->integer;
    }
    if (reply) freeReplyObject(reply);
    return ret;
}

std::string RedisClient::rpop(const std::string& listKey) {
    if (!m_connected) return "";
    redisReply* reply = (redisReply*)redisCommand(m_context, "RPOP %s", listKey.c_str());
    std::string value;
    if (reply && reply->type == REDIS_REPLY_STRING) {
        value = reply->str;
    }
    if (reply) freeReplyObject(reply);
    return value;
}

bool RedisClient::ltrim(const std::string& listKey, int start, int stop) {
    if (!m_connected) return false;
    redisReply* reply = (redisReply*)redisCommand(m_context, "LTRIM %s %d %d", listKey.c_str(), start, stop);
    bool ret = reply && reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK";
    if (reply) freeReplyObject(reply);
    return ret;
}

std::vector<std::string> RedisClient::lrange(const std::string& listKey, int start, int stop) {
    std::vector<std::string> result;
    if (!m_connected) return result;
    redisReply* reply = (redisReply*)redisCommand(m_context, "LRANGE %s %d %d", listKey.c_str(), start, stop);
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; ++i) {
            if (reply->element[i]->type == REDIS_REPLY_STRING) {
                result.push_back(reply->element[i]->str);
            }
        }
    }
    if (reply) freeReplyObject(reply);
    return result;
}