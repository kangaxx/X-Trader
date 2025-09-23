#!/usr/bin/bash
lastcomm demo | awk '{
	secs = $(NF-5);
	h = int(secs / 3600);
	m = int((secs % 3600) / 60);
	s = int(secs % 60);
	$(NF - 3) = sprintf("%02d 小时%02d 分钟%02d 秒", h, m, s);
	print $0
}'
