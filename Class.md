# Class #

Instances of this class are used to interact with a serializer.

  * HandlerSocket
  * HandlerSocketIndex

# Synopsis #
## HandlerSocket ##
```
HandlerSocket {
  /* Constants */
  const string PRIMARY = "PRIMARY";
  const string UPDATE = "U";
  const string DELETE = "D";

  /* Methods */
  public HandlerSocket::__construct( string $host, string $port [, array $options ] )
  public bool HandlerSocket::openIndex( long $id, string $db, string $table, string $index, string $field [, array $filter = array() ] )
  public bool HandlerSocket::auth( string $key [, string $type ] )
  public mixed HandlerSocket::executeSingle( long $id, string $operate, array $criteria [, long $limit = 1, long $offset = 0, string $update = null, array $values = array(), array $filters = array(), long $in_key = -1, array $in_values = array() ] )
  public array HandlerSocket::executeMulti( array $args )
  public mixed HandlerSocket::executeUpdate( long $id, string $operate, array $criteria, array $values [, long $limit = 1, long $offset = 0, array $filters = array(), long $in_key  = -1, array $in_values = array() ] )
  public mixed HandlerSocket::executeDelete( long $id, string $operate, array $criteria [, long $limit = 1, long $offset = 0, array $filters = array(), long $in_key = -1, array $in_values = array() ] )
  public mixed HandlerSocket::executeInsert( long $id, array $field )
  public mixed HandlerSocket::getError( void )
  public HandlerSocketIndex HandlerSocket::createIndex( long $id, string $db, string $table, string $index, string|array $fields [, array $options = array() ] )
}
```

## HandlerSocketIndex ##
```
HandlerSocketIndex {
  /* Fields */
  protected string _db;
  protected string _table;
  protected string _name;
  protected mixed _field;

  /* Methods */
  public HandlerSocketIndex::__construct( HandlerSocket $hs, long $id, string $db, string $table, string $index, string|array $fields [, array $options = array() ] )
  public long HandlerSocketIndex::getId( void )
  public string HandlerSocketIndex::getDatabase( void )
  public string HandlerSocketIndex::getTable( void )
  public string HandlerSocketIndex::getName( void )
  public array HandlerSocketIndex::getField( void )
  public array HandlerSocketIndex::getFilter( void )
  public array HandlerSocketIndex::getOperator( void )
  public mixed HandlerSocketIndex::getError( void )
  public mixed HandlerSocketIndex::find ( string|array $query [, long $limit = 1, long $offset = 0, array $options = array() ] )
  public mixed HandlerSocketIndex::insert( mixed $var [, string $... ] )
  public mixed HandlerSocketIndex::update( string|array $query, string|array $update [, long $limit = 1, long $offset = 0, array $options = array() ] )
  public mixed HandlerSocketIndex::remove( string|array $query [, long $limit = 1, long $offset = 0, array $options = array() ] )
  public array HandlerSocketIndex::multi( array $args )
}
```