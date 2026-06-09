# MySQL 8.0 启动与登录错误排查记录

时间：2026-06-09

环境：

- Windows
- MySQL Community Server 8.0.46
- 安装目录：`C:\Program Files\MySQL\MySQL Server 8.0\bin`
- 配置文件：`C:\ProgramData\MySQL\MySQL Server 8.0\my.ini`
- 服务名：`MySQL80`
- 默认端口：`3306`

## 一、最初的问题

在 CMD 中执行：

```bat
C:\Program Files\MySQL\MySQL Server 8.0\bin>mysql -u root -p
Enter password: *************
ERROR 2003 (HY000): Can't connect to MySQL server on 'localhost:3306' (10061)
```

### 含义

`ERROR 2003 (HY000)` 表示 MySQL 客户端无法连接 MySQL 服务端。

`localhost:3306` 是客户端要连接的地址和端口。

`10061` 在 Windows 上通常表示：

```text
目标端口没有服务在监听，连接被拒绝。
```

也就是说，`mysql.exe` 客户端本身能运行，但 MySQL 服务端 `mysqld.exe` 当时没有正常运行在 `3306` 端口上。

## 二、检查服务状态

检查结果显示本机有多个 MySQL 服务：

```text
Name       DisplayName  Status   StartType
----       -----------  ------   ---------
MySQL32234 MySQL32234   Stopped  Automatic
MySQL80    MySQL80      Stopped  Automatic
MySQL84    MySQL84      Stopped  Automatic
MySQL97    MySQL97      Stopped  Automatic
MySQL99    MySQL99      Stopped  Automatic
MySQLs     MySQLs       Stopped  Automatic
```

其中 `MySQL80` 对应当前使用的 MySQL 8.0：

```text
SERVICE_NAME: MySQL80
BINARY_PATH_NAME:
"C:\Program Files\MySQL\MySQL Server 8.0\bin\mysqld.exe" --defaults-file="C:\ProgramData\MySQL\MySQL Server 8.0\my.ini" MySQL80
```

配置文件中确认端口为：

```ini
port=3306
datadir=C:/ProgramData/MySQL/MySQL Server 8.0\Data
log-error="WT的电脑.err"
log-bin="WT的电脑-bin"
```

## 三、启动服务失败

管理员 PowerShell / CMD 中执行：

```bat
net start MySQL80
```

返回：

```text
MySQL80 服务正在启动 .
MySQL80 服务无法启动。

服务没有报告任何错误。

请键入 NET HELPMSG 3534 以获得更多的帮助。
```

### 含义

这说明 Windows 服务管理器尝试启动 `MySQL80`，但 `mysqld.exe` 启动后很快退出了。

Windows 没有拿到具体错误，因此需要看 MySQL 自己的错误日志，或者用前台模式启动：

```powershell
cd "C:\Program Files\MySQL\MySQL Server 8.0\bin"
.\mysqld.exe --defaults-file="C:\ProgramData\MySQL\MySQL Server 8.0\my.ini" --console
```

## 四、第一处真正错误：binlog 文件名乱码

错误日志中出现：

```text
2026-06-09T06:09:20.964592Z 0 [System] [MY-010116] [Server] C:\Program Files\MySQL\MySQL Server 8.0\bin\mysqld.exe (mysqld 8.0.46) starting as process 43700
mysqld: File '.\WT鐨勭數鑴?bin.index' not found (OS errno 2 - No such file or directory)
2026-06-09T06:09:20.970987Z 0 [ERROR] [MY-010119] [Server] Aborting
2026-06-09T06:09:20.971138Z 0 [System] [MY-010910] [Server] C:\Program Files\MySQL\MySQL Server 8.0\bin\mysqld.exe: Shutdown complete (mysqld 8.0.46)  MySQL Community Server - GPL.
```

### 原因

配置文件中原来有：

```ini
log-bin="WT的电脑-bin"
```

MySQL 启动时需要根据 `log-bin` 生成或读取二进制日志索引文件：

```text
WT的电脑-bin.index
```

但因为配置中使用了中文文件名，MySQL 或 Windows 当前编码环境把中文解析成了乱码：

```text
WT鐨勭數鑴?bin.index
```

其中 `?` 在 Windows 文件名中也是非法字符，最终导致 MySQL 找不到这个索引文件并中止启动。

### 修复方法

用管理员权限打开配置文件：

```powershell
notepad "C:\ProgramData\MySQL\MySQL Server 8.0\my.ini"
```

将中文日志文件名改成英文：

```ini
log-error=mysql80.err
log-bin=mysql-bin
log-bin-index=mysql-bin.index
```

注意：不要删除 `Data` 目录，也不要随意删除 InnoDB 文件。

## 五、第二处警告：slow log 文件名乱码

后来前台启动时出现：

```text
2026-06-09T06:27:16.181850Z 0 [System] [MY-010116] [Server] C:\Program Files\MySQL\MySQL Server 8.0\bin\mysqld.exe (mysqld 8.0.46) starting as process 37996
2026-06-09T06:27:16.195008Z 1 [System] [MY-013576] [InnoDB] InnoDB initialization has started.
2026-06-09T06:27:16.437196Z 1 [System] [MY-013577] [InnoDB] InnoDB initialization has ended.
mysqld: File '.\WT鐨勭數鑴?slow.log' not found (OS errno 2 - No such file or directory)
2026-06-09T06:27:16.592807Z 0 [ERROR] [MY-011263] [Server] Could not use WT鐨勭數鑴?slow.log for logging (error 2 - No such file or directory). Turning logging off for the server process. To turn it on again: fix the cause, then either restart the query logging by using "SET GLOBAL SLOW_QUERY_LOG=ON" or restart the MySQL server.
2026-06-09T06:27:16.664278Z 0 [Warning] [MY-010068] [Server] CA certificate ca.pem is self signed.
2026-06-09T06:27:16.664430Z 0 [System] [MY-013602] [Server] Channel mysql_main configured to support TLS. Encrypted connections are now supported for this channel.
2026-06-09T06:27:16.706642Z 7 [Warning] [MY-013360] [Server] Plugin mysql_native_password reported: ''mysql_native_password' is deprecated and will be removed in a future release. Please use caching_sha2_password instead'
2026-06-09T06:27:16.721892Z 0 [System] [MY-011323] [Server] X Plugin ready for connections. Bind-address: '::' port: 33060
2026-06-09T06:27:16.721967Z 0 [System] [MY-010931] [Server] C:\Program Files\MySQL\MySQL Server 8.0\bin\mysqld.exe: ready for connections. Version: '8.0.46'  socket: ''  port: 3306  MySQL Community Server - GPL.
```

### 判断

这次 MySQL 已经启动成功，因为最后有：

```text
ready for connections ... port: 3306
```

慢查询日志乱码只是警告：

```text
Could not use WT鐨勭數鑴?slow.log for logging
```

它会导致慢查询日志关闭，但不会阻止 MySQL 启动。

### 建议修复

在 `my.ini` 中查找类似：

```ini
slow_query_log_file=...
```

如果值包含中文，改成英文：

```ini
slow_query_log_file=mysql-slow.log
```

## 六、启动成功后登录失败

服务启动成功后，执行：

```bat
mysql -u root -p
```

返回：

```text
ERROR 1045 (28000): Access denied for user 'root'@'localhost' (using password: YES)
```

### 含义

`ERROR 1045` 表示客户端已经连到了 MySQL 服务，但账号认证失败。

常见原因：

- root 密码输入错误
- root 密码忘记了
- 连接的是另一个 MySQL 实例
- root 账号的认证方式或主机匹配不符合当前登录方式

这和 `ERROR 2003` 不同：

- `ERROR 2003`：服务没连上，通常是 MySQL 没启动或端口不通
- `ERROR 1045`：服务已经连上，但账号或密码不对

## 七、重置 root 密码

先停止服务：

```powershell
net stop MySQL80
```

确认没有 `mysqld.exe` 进程：

```powershell
tasklist | findstr mysqld
```

创建临时 SQL 文件：

```text
C:\mysql-reset.txt
```

内容：

```sql
ALTER USER 'root'@'localhost' IDENTIFIED BY 'Root@123456';
```

然后前台启动 MySQL，并让它执行这个初始化 SQL：

```powershell
cd "C:\Program Files\MySQL\MySQL Server 8.0\bin"
.\mysqld.exe --defaults-file="C:\ProgramData\MySQL\MySQL Server 8.0\my.ini" --init-file="C:\mysql-reset.txt" --console
```

注意：PowerShell 中必须写 `.\mysqld.exe`，不能只写 `mysqld`。

原因是 PowerShell 默认不会从当前目录加载命令。如果程序就在当前目录，需要加：

```powershell
.\程序名.exe
```

看到：

```text
ready for connections ... port: 3306
```

说明 MySQL 启动成功，`--init-file` 中的 SQL 通常已经执行。

然后按 `Ctrl + C` 停止前台 MySQL，等看到：

```text
Shutdown complete
```

再启动 Windows 服务：

```powershell
net start MySQL80
```

最后登录：

```powershell
mysql -u root -p
```

输入新密码：

```text
Root@123456
```

成功。

## 八、删除临时密码文件

因为 `C:\mysql-reset.txt` 中有明文密码，重置成功后应该删除。

检查文件是否存在：

```powershell
Test-Path C:\mysql-reset.txt
```

如果返回 `True`，删除：

```powershell
Remove-Item C:\mysql-reset.txt
```

如果找不到，可以搜索：

```powershell
Get-ChildItem C:\Users\17757 -Filter mysql-reset.txt -Recurse -ErrorAction SilentlyContinue
```

找到后删除对应文件。

## 九、以后遇到类似问题怎么判断

### 1. 先看错误码

```text
ERROR 2003
```

优先判断 MySQL 服务是否启动、3306 是否监听。

```text
ERROR 1045
```

说明 MySQL 服务端已连接成功，问题转为账号密码或权限认证。

### 2. 检查服务

```powershell
Get-Service MySQL80
```

或者：

```powershell
net start MySQL80
```

### 3. 检查端口

```powershell
netstat -ano | findstr 3306
```

如果没有任何输出，说明 3306 没有服务在监听。

### 4. 前台启动看真实错误

```powershell
cd "C:\Program Files\MySQL\MySQL Server 8.0\bin"
.\mysqld.exe --defaults-file="C:\ProgramData\MySQL\MySQL Server 8.0\my.ini" --console
```

这是最直接的排错方法。

### 5. 看 MySQL 错误日志

配置文件里：

```ini
log-error=...
```

对应的文件通常在：

```text
C:\ProgramData\MySQL\MySQL Server 8.0\Data
```

读取最后部分：

```powershell
Get-Content "日志文件路径" -Tail 120
```

### 6. 配置文件尽量避免中文文件名

建议这些配置都用英文：

```ini
log-error=mysql80.err
log-bin=mysql-bin
log-bin-index=mysql-bin.index
slow_query_log_file=mysql-slow.log
```

避免使用类似：

```ini
WT的电脑.err
WT的电脑-bin
WT的电脑-slow.log
```

因为在某些编码环境下，中文会变成乱码，造成 MySQL 找不到日志文件或索引文件。

## 十、本次最终结论

本次一共有两个问题：

1. MySQL 服务无法启动
   - 直接原因：`my.ini` 中 `log-bin` 使用中文文件名。
   - 表现：`ERROR 2003`，3306 没有监听。
   - 修复：将 binlog、error log、slow log 等文件名改为英文。

2. MySQL 启动后 root 登录失败
   - 直接原因：root 密码不正确或已忘记。
   - 表现：`ERROR 1045`。
   - 修复：使用 `--init-file` 重置 root 密码，并删除临时密码文件。

最终状态：

- `MySQL80` 启动成功
- `3306` 可连接
- `root` 可以使用新密码登录
