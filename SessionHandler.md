# About #

PHP implementation of a HandlerSocket plugin for MySQL session wrapper.

# Usage #

Database tables are required.

```
CREATE DATABASE `session` DEFAULT CHARACTER SET utf8;

CREATE TABLE `php_session` (
  `id`       varchar(32) NOT NULL DEFAULT '',
  `modified` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `data`     text,
  PRIMARY KEY (`id`),
  KEY `modified` (`modified`)
) ENGINE=InnoDB DEFAULT CHARACTER SET utf8;
```

Basic usage for a single HandlerSocket plugin for MySQL database on localhost is as simple as the following:

```
<?php
require_once('HandlerSocketSession.php');
HandlerSocketSession::start();
?>
```

You can change the connection settings set to the class by passing an array.

For more information please see the source code.

[HandlerSocketSession.php](http://code.google.com/p/php-handlersocket/source/browse/trunk/examples/HandlerSocketSession.php)