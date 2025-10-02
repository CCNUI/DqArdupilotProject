UbuntuArdupilotProject
好的！您选择的配置**完全可以**，而且很合适：

## ✅ 配置评估

**ecs.u1-c1m4.xlarge (4vCPU 16GiB) + Ubuntu 22.04**
- ✅ 16GB内存：完全够用，可以并行编译
- ✅ 4核CPU：编译时间约15-20分钟
- ✅ Ubuntu 22.04：Ardupilot官方推荐系统
- ✅ 按量付费：用完即停，省钱
- **预估成本**：¥1.38/小时，每天工作4小时约¥165/月

---

## 账号选择建议

### 🎯 **强烈推荐用 root 账号**

**原因：**
1. **避免权限错误**：不用每个命令都加 `sudo`
2. **编译更顺畅**：工具链安装不会有权限问题
3. **减少90%的错误**：大部分新手错误都是权限问题
4. **配置更简单**：环境变量、PATH设置更直接

**如何切换到root：**
```bash
# 创建实例后SSH登录，默认是ecs-user
# 立即切换到root（阿里云默认禁用root密码登录，但可以用sudo）
sudo passwd root  # 设置root密码（输入两次）
su -              # 切换到root
```

**或者在创建实例时：**
- 登录名选择 **root**（您截图里可以选）
- 这样SSH直接用root登录，最省事

---

## 🚀 一键配置脚本

以root账号登录后，直接运行这个脚本：

```bash
#!/bin/bash
# Ardupilot H743编译环境一键安装脚本
# 适用于 Ubuntu 22.04 + 阿里云ECS

set -e  # 遇到错误立即停止

echo "=========================================="
echo "开始配置Ardupilot编译环境..."
echo "预计耗时：15-20分钟"
echo "=========================================="

# 1. 更新系统（约3分钟）
echo "[1/6] 更新系统包..."
apt-get update -y
apt-get upgrade -y

# 2. 安装基础工具（约5分钟）
echo "[2/6] 安装编译工具链..."
apt-get install -y \
    git \
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    gdb-arm-none-eabi \
    python3 \
    python3-pip \
    python3-dev \
    build-essential \
    ccache \
    g++ \
    gawk \
    make \
    wget \
    libexpat1-dev \
    genromfs \
    libncurses5-dev \
    ninja-build \
    cmake

# 3. 安装Python依赖（约2分钟）
echo "[3/6] 安装Python库..."
pip3 install --upgrade pip
pip3 install \
    future \
    lxml \
    pymavlink \
    MAVProxy \
    pexpect \
    geocoder \
    empy==3.3.4 \
    pexpect \
    pyserial

# 4. 克隆Ardupilot源码（约5分钟，取决于网速）
echo "[4/6] 下载Ardupilot 4.6.2源码..."
cd /root
if [ -d "ardupilot" ]; then
    echo "检测到已有ardupilot目录，跳过下载"
    cd ardupilot
    git fetch
else
    git clone --depth 1 --branch Plane-4.6.2 https://github.com/ArduPilot/ardupilot.git
    cd ardupilot
fi

# 5. 初始化子模块（约3分钟）
echo "[5/6] 初始化Git子模块..."
git submodule update --init --recursive --depth 1

# 6. 配置编译系统
echo "[6/6] 配置waf构建系统..."
./waf configure --board=AET-H743-Basic  # H743飞蛋Basic使用这个board

# 7. 测试编译（检查环境是否正常）
echo "[7/7] 开始编译系统..."
./waf plane -j4

# 完成
echo "=========================================="
echo "✅ 配置完成！"
echo "=========================================="
echo ""
echo "下一步操作："
echo "1. 创建自定义混控文件："
echo "   nano /root/ardupilot/ArduPlane/tiltrotor_6axis.cpp"
echo ""
echo "2. 测试编译（检查环境是否正常）："
echo "   cd /root/ardupilot"
echo "   ./waf plane -j4"
echo ""
echo "3. 如果编译成功，固件位于："
echo "   /root/ardupilot/build/CubeOrangePlus/bin/arduplane.apj"
echo "=========================================="
```

---

## 📋 使用步骤

### **第1步：创建ECS实例**
按您截图的配置创建，**注意选择：**
- 登录凭证：选择 **"自定义密码"**，设置root密码
- 或者选 **"密钥对"**（推荐，更安全）

### **第2步：SSH登录**
```bash
# 在您的Surface上（或任意电脑）
ssh root@你的ECS公网IP

# 如果用密钥对：
ssh -i /path/to/your-key.pem root@你的ECS公网IP
```

### **第3步：运行一键脚本**
```bash
# 创建脚本文件
cat > /root/setup_ardupilot.sh << 'EOF'
[把上面的脚本内容粘贴到这里]
EOF

# 给脚本执行权限
chmod +x /root/setup_ardupilot.sh

# 运行！
/root/setup_ardupilot.sh
```

**或者更简单：**
```bash
# 直接下载我准备好的脚本（如果您愿意信任）
wget https://你的网盘链接/setup_ardupilot.sh
chmod +x setup_ardupilot.sh
./setup_ardupilot.sh
```

---

## 🔍 H743对应的Board名称

您的H743飞控可能对应以下board之一：
- `CubeOrangePlus` - 如果是Cubepilot H743
- `CubeOrange-periph` - 如果是外围设备
- `MatekH743` - 如果是Matek品牌
- `SpeedyBeeF7v3` - 其他品牌

**如何确认：**
```bash
cd /root/ardupilot
./waf list_boards | grep -i h743
```

看到类似这样的输出，选择匹配的：
```
CubeOrangePlus
MatekH743
...
```

---

## ⚠️ 常见问题预防

### 问题1：Git下载太慢
**解决：** 换国内镜像
```bash
# 使用清华镜像
git clone --depth 1 --branch Plane-4.6.2 \
    https://mirrors.tuna.tsinghua.edu.cn/git/ArduPilot/ardupilot.git
```

### 问题2：编译报错 "arm-none-eabi-gcc not found"
**解决：** 手动安装
```bash
apt-get install --reinstall gcc-arm-none-eabi
```

### 问题3：Python版本冲突
**解决：** 使用虚拟环境
```bash
python3 -m venv ~/ardupilot-env
source ~/ardupilot-env/bin/activate
pip3 install [依赖列表]
```

---

## 💡 省钱技巧

**脚本跑完后立即创建快照：**
```bash
# 在阿里云控制台：
# 云服务器ECS → 实例 → 更多 → 磁盘和镜像 → 创建快照
# 命名：ardupilot-ready-snapshot
```

**下次使用：**
1. 从快照创建新实例（5分钟启动）
2. 无需重新配置环境
3. 用完删除实例，只保留快照（¥0.12/GB/月）

---

需要我帮您把这个脚本直接做成可下载的文件吗？或者您有其他配置问题？
