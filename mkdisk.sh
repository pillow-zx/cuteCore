#!/usr/bin/env bash
# mkdisk.sh — 重新创建 disk.img（用于测试前的环境重置）
# 规格：64 MiB ext2，块大小 4096，inode 大小 256，预置 hello.txt
set -e

IMG=disk.img
SIZE_MB=64
trap 'rm -f temp.txt' EXIT

echo "Creating ${IMG} (${SIZE_MB} MiB ext2)..."

# 创建空白镜像
dd if=/dev/zero of="${IMG}" bs=1M count=${SIZE_MB} status=none

# 格式化为 ext2
# -b 4096        : 块大小 4096（s_log_block_size=2）
# -I 256         : inode 大小 256
# -O none        : 关闭所有可选特性，保持 good-old rev 兼容
# -t ext2        : 明确指定 ext2
mkfs.ext2 -b 4096 -I 256 -t ext2 -q "${IMG}"

# 写入 hello.txt（原始内容，供 test_read 初次运行使用）
# 使用 debugfs 直接写入，无需 root 或 mount
printf 'Hello, cuteCore! This is a read test.\nLine 2: ext2 over virtio works!\nLine 3: The quick brown fox jumps over the lazy dog.\n' \
    > temp.txt

debugfs -w "${IMG}" -R "write temp.txt hello.txt"

rm -f temp.txt
trap - EXIT
