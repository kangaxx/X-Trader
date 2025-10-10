#!/bin/bash
# 脚本功能：将 /root/X-Trader/ 内容复制到 simnow-Trader2，排除 src/main.cpp
# 执行方式：chmod +x copy_trader.sh && ./copy_trader.sh

# 定义源目录、目标目录和排除规则（可根据需求修改）
SOURCE_DIR="/root/X-Trader/"
TARGET_DIR="/root/X-Trader/simnow-Trader2/"
EXCLUDE_FILE="src/main.cpp"

# 输出脚本执行信息
echo "========================================"
echo "开始执行文件复制任务"
echo "源目录：$SOURCE_DIR"
echo "目标目录：$TARGET_DIR"
echo "排除文件：$EXCLUDE_FILE"
echo "========================================"

# 检查源目录是否存在
if [ ! -d "$SOURCE_DIR" ]; then
    echo "❌ 错误：源目录 $SOURCE_DIR 不存在，请检查路径是否正确"
    exit 1  # 退出脚本，状态码 1 表示执行失败
fi

# 执行 rsync 复制命令
echo -e "\n🔄 正在复制文件（跳过 $EXCLUDE_FILE）..."
rsync -av --exclude="$EXCLUDE_FILE" "$SOURCE_DIR" "$TARGET_DIR"

# 检查命令执行结果
if [ $? -eq 0 ]; then
    echo -e "\n✅ 复制完成！目标目录已更新（未覆盖 $TARGET_DIR$EXCLUDE_FILE）"
else
    echo -e "\n❌ 复制失败！请检查目录权限或 rsync 命令输出"
    exit 1
fi

echo -e "\n========================================"
echo "脚本执行结束"
echo "========================================"