#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H

#include <string>
#include <memory>
#include <hiredis/hiredis.h>

// Redis客户端单例类
class RedisClient {
public:
	RedisClient(const RedisClient&) = delete;
	RedisClient& operator=(const RedisClient&) = delete;

	static RedisClient& getInstance();

	bool connect(const std::string& host = "127.0.0.1", int port = 6379, int timeout = 5000);
	void disconnect();

	bool isConnected() const;

	bool set(const std::string& key, const std::string& value);

	std::string get(const std::string& key);
	long long del(const std::string& key);

	bool exists(const std::string& key);
	bool expire(const std::string& key, int seconds);

	long long lpush(const std::string& listKey, const std::string& value);

	std::string rpop(const std::string& listKey);

private:
	RedisClient();
	~RedisClient();
	redisContext* m_context;
	bool m_connected;
};

#endif
