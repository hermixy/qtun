# Supported Lib

1. base
2. table
3. string
4. io

# Global

1. [\_\_conf\_\_](#__conf__)
2. [\_syslog](#_syslog)
3. [qtun.state](#qtunstate)
4. [qtun.conf](#qtunconf)
5. [qtun.log](#qtunlog)

## \_\_conf\_\_

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | user\_data                    |
| **desc**   | It's supported to get `library_conf_t` in c language. So you can't use it in your lua script. |

## \_syslog

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | function                      |
| **desc**   | Log for system log.           |

	_syslog(level, fmt, ...)
	
*level*: write log when [qtun.state.log_level](#qtunstatelog_level) >= level.

*fmt*: format string for follow arguments.

## qtun.state

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | table                         |
| **desc**   | It's the object for global variable `this`, it has many properties follow. |

### qtun.state.msg\_ident

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | number                        |
| **access** | readonly                      |
| **desc**   | Ident for send message.       |

### qtun.state.msg\_ttl

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | number                        |
| **access** | readonly                      |
| **desc**   | TTL for recv message.         |

### qtun.state.localip

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | string                        |
| **access** | readonly                      |
| **desc**   | Virtual LAN ip address.       |

### qtun.state.netmask

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | number                        |
| **access** | readonly                      |
| **desc**   | Netmask for Virtual LAN.      |

### qtun.state.log\_level

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | number                        |
| **access** | readonly                      |
| **desc**   | Log level in [`qtun.log.LOG_EMERG`](#log_levels), [`qtun.log.LOG_ALERT`](#log_levels), [`qtun.log.LOG_CRIT`](#log_levels), [`qtun.log.LOG_ERR`](#log_levels), [`qtun.log.LOG_WARNING`](#log_levels), [`qtun.log.LOG_NOTICE`](#log_levels), [`qtun.log.LOG_INFO`](#log_levels) and [`qtun.log.LOG_DEBUG`](#log_levels). |

### qtun.state.little\_endian

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | boolean                       |
| **access** | readonly                      |
| **desc**   | Return true when this machine is little endian encoding. |

### qtun.state.internal\_mtu

| key        | value                          |
| ---------- | ------------------------------ |
| **type**   | number                         |
| **access** | readonly                       |
| **desc**   | Limit for send message length. |

### qtun.state.use\_udp

| key        | value                                            |
| ---------- | ------------------------------------------------ |
| **type**   | boolean                                          |
| **access** | readonly                                         |
| **desc**   | Return true when they are using udp for connect. |

### qtun.state.aes\_key

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | string or nil                 |
| **access** | readonly                      |
| **desc**   | AES encrypt key, must be `128`, `192` or `256` bit. `nil` when not using AES encryption. |

### qtun.state.aes\_key\_len

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | number                        |
| **access** | readonly                      |
| **desc**   | AES encrypt key length, must be `128`, `192` or `256`. `0` when not using AES encryption. |

### qtun.state.aes\_iv

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | string or nil                 |
| **access** | readonly                      |
| **desc**   | AES encrypt initial vector, must be `128` bit. `nil` when not using AES encryption. |

### qtun.state.des\_key

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | string or nil                 |
| **access** | readonly                      |
| **desc**   | DES encrypt key, must be `64`, `128` or `192` bit. `nil` when not using DES encription. |

### qtun.state.des\_key\_len

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | number                        |
| **access** | readonly                      |
| **desc**   | DES encrypt key length, must be `8`, `16` or `24`. `0` when not using DES encryption. |

### qtun.state.des\_iv

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | string or nil                 |
| **access** | readonly                      |
| **desc**   | DES encrypt initial vector, must be `64` bit. `nil` when not using DES encryption. |

### qtun.state.compress

| key        | value                            |
| ---------- | -------------------------------- |
| **type**   | number                           |
| **access** | readonly                         |
| **desc**   | bit mask for the compress masks. |

	#define MSG_COMPRESS_GZIP_ID         1

### qtun.state.encrypt

| key        | value                           |
| ---------- | ------------------------------- |
| **type**   | number                          |
| **access** | readonly                        |
| **desc**   | bit mask for the encrypt masks. |

	#define MSG_ENCRYPT_AES_ID           1
	#define MSG_ENCRYPT_DES_ID           2

## qtun.conf

| key        | value                                           |
| ---------- | ----------------------------------------------- |
| **type**   | table                                           |
| **desc**   | It's the object, it has many properties follow. |

### qtun.conf.conf\_file

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | string                        |
| **access** | readonly                      |
| **desc**   | path for config file.         |

### qtun.conf.localip

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | string                        |
| **access** | read and write                |
| **desc**   | Virtual LAN ip address.       |

### qtun.conf.netmask

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | number                        |
| **access** | read and write                |
| **desc**   | Netmask for Virtual LAN.      |

### qtun.conf.log\_level

| key        | value          |
| ---------- | -------------- |
| **type**   | number         |
| **access** | read and write |
| **desc**   | Log level in [`qtun.log.LOG_EMERG`](#log_levels), [`qtun.log.LOG_ALERT`](#log_levels), [`qtun.log.LOG_CRIT`](#log_levels), [`qtun.log.LOG_ERR`](#log_levels), [`qtun.log.LOG_WARNING`](#log_levels), [`qtun.log.LOG_NOTICE`](#log_levels), [`qtun.log.LOG_INFO`](#log_levels) and [`qtun.log.LOG_DEBUG`](#log_levels).|

### qtun.conf.internal\_mtu

| key        | value                          |
| ---------- | ------------------------------ |
| **type**   | number                         |
| **access** | read and write                 |
| **desc**   | Limit for send message length. |

### qtun.conf.dev\_symbol

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | string                        |
| **access** | readonly                      |
| **desc**   | It's only use for `windows` to show the symbol path for an adapter driver. |

### qtun.conf.dev\_name

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | string                        |
| **access** | readonly                      |
| **desc**   | It's only use for `windows` to show the adapter name in `control panel -> network connectioins`. |

### qtun.conf.use\_udp

| key        | value                             |
| ---------- | --------------------------------- |
| **type**   | boolean                           |
| **access** | read and write                    |
| **desc**   | Set or get if use udp connection. |

### qtun.conf.use\_gzip

| key        | value                               |
| ---------- | ----------------------------------- |
| **type**   | boolean                             |
| **access** | read and write                      |
| **desc**   | Set or get if use gzip compression. |

### qtun.conf.aes\_file

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | string or nil                 |
| **access** | read and write                |
| **desc**   | Set AES encryption key file, when not using AES encryption, it will return nil. |

### qtun.conf.des\_file

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | string or nil                 |
| **access** | read and write                |
| **desc**   | Set DES encryption key file, when not using DES encryption, it will return nil. |

## qtun.log

| key        | value                         |
| ---------- | ----------------------------- |
| **type**   | table                         |
| **desc**   | Object for syslog.            |

### qtun.log.syslog

| key        | value                          |
| ---------- | ------------------------------ |
| **type**   | function                       |
| **desc**   | alias for [\_syslog](_syslog). |

### log\_levels

| key                      | value |
| ------------------------ | :---: |
| **qtun.log.LOG_EMERG**   | 0     |
| **qtun.log.LOG_ALERT**   | 1     |
| **qtun.log.LOG_CRIT**    | 2     |
| **qtun.log.LOG_ERR**     | 3     |
| **qtun.log.LOG_WARNING** | 4     |
| **qtun.log.LOG_NOTICE**  | 5     |
| **qtun.log.LOG_INFO**    | 6     |
| **qtun.log.LOG_DEBUG**   | 7     |
