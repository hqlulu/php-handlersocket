# Summary #
This project has [moved to GitHub](https://github.com/kjdev/php-ext-handlersocketi).

This extension provide API for communicating with [HandlerSocket plugin for MySQL](http://github.com/ahiguti/HandlerSocket-Plugin-for-MySQL).

libhsclient binding for PHP.

## notice ##
It may be defective for a test version.

## implemented ##
| implemented | configure options | source file |
|:------------|:------------------|:------------|
| hsclient    | none (**default**) | handlersocket.cc |
| native      | --disable-handlersocket-hsclient | handlersocet.c |

# Installation #
required to compile the libhsclient.

```
$ phpize
$ ./configure  or  ./configure --disable-handlersocket-hsclient
$ make
# make install
```

A successful install will have created handlersocket.so and put it into the PHP extensions directory. You'll need to and adjust php.ini and add an extension=handlersocket.so line before you can use the extension.

# Class synopsis #
[>> Class](http://code.google.com/p/php-handlersocket/wiki/Class)

# Example #
```
<?php
$host = 'localhost';
$port = 9998;
$port_wr = 9999;
$dbname = 'hstestdb';
$table = 'hstesttbl';

//GET
$hs = new HandlerSocket($host, $port);
if (!($hs->openIndex(1, $dbname, $table, HandlerSocket::PRIMARY, 'k,v')))
{
    echo $hs->getError(), PHP_EOL;
    die();
}

$retval = $hs->executeSingle(1, '=', array('k1'), 1, 0);

var_dump($retval);

$retval = $hs->executeMulti(
    array(array(1, '=', array('k1'), 1, 0),
          array(1, '=', array('k2'), 1, 0)));

var_dump($retval);

unset($hs);


//UPDATE
$hs = new HandlerSocket($host, $port_wr);
if (!($hs->openIndex(2, $dbname, $table, '', 'v')))
{
    echo $hs->getError(), PHP_EOL;
    die();
}

if ($hs->executeUpdate(2, '=', array('k1'), array('V1'), 1, 0) === false)
{
    echo $hs->getError(), PHP_EOL;
    die();
}

unset($hs);


//INSERT
$hs = new HandlerSocket($host, $port_wr);
if (!($hs->openIndex(3, $dbname, $table, '', 'k,v')))
{
    echo $hs->getError(), PHP_EOL;
    die();
}

if ($hs->executeInsert(3, array('k2', 'v2')) === false)
{
    echo $hs->getError(), PHP_EOL;
}
if ($hs->executeInsert(3, array('k3', 'v3')) === false)
{
    echo 'A', $hs->getError(), PHP_EOL;
}
if ($hs->executeInsert(3, array('k4', 'v4')) === false)
{
    echo 'B', $hs->getError(), PHP_EOL;
}

unset($hs);


//DELETE
$hs = new HandlerSocket($host, $port_wr);
if (!($hs->openIndex(4, $dbname, $table, '', '')))
{
    echo $hs->getError(), PHP_EOL;
    die();
}

if ($hs->executeDelete(4, '=', array('k2')) === false)
{
    echo $hs->getError(), PHP_EOL;
    die();
}
```