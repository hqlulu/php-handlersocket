--TEST--
libmysql
--SKIPIF--
<?php
ob_start();
phpinfo();
$str = ob_get_clean();
$array = explode("\n", $str);
$zts = false;
foreach ($array as $key => $val)
{
    if (strstr($val, 'Thread Safety') != false)
    {
        $retval = explode(' ', $val);
        if (strcmp($retval[3], 'enabled') == 0)
        {
            $zts = true;
        }
    }
}
if ($zts)
{
    echo 'skip tests in Thread Safety disabled';
}
--FILE--
<?php
require_once dirname(__FILE__) . '/../common/config.php';

$mysql = get_mysql_connection();

init_mysql_testdb($mysql);

$table = 'hstesttbl';
$tablesize = 100;
$sql = sprintf(
    'CREATE TABLE %s (k varchar(30) primary key, v varchar(30) not null) ' .
    'Engine = innodb', mysql_real_escape_string($table));
if (!mysql_query($sql, $mysql))
{
    die(mysql_error());
}

srand(999);

$valmap = array();

for ($i = 0; $i < $tablesize; ++$i)
{
    $k = 'k' . $i;
    $v = 'v' . (int)rand(0, 1000) . $i;

    $sql = sprintf(
        'INSERT INTO ' . $table . ' values (\'%s\', \'%s\')',
        mysql_real_escape_string($k),
        mysql_real_escape_string($v));
    if (!mysql_query($sql, $mysql))
    {
        break;
    }

    $valmap[$k] = $v;
}

$sql = 'SELECT k,v FROM ' . $table . ' ORDER BY k';
$result = mysql_query($sql, $mysql);
if ($result)
{
    while ($row = mysql_fetch_assoc($result))
    {
        echo $row['k'], ' ', $row['v'], PHP_EOL;
    }
    mysql_free_result($result);
}

mysql_close($mysql);

--EXPECT--
k0 v20
k1 v6441
k10 v92910
k11 v21111
k12 v37712
k13 v19213
k14 v9014
k15 v14515
k16 v40116
k17 v48017
k18 v53518
k19 v66619
k2 v8252
k20 v26420
k21 v97221
k22 v11322
k23 v72123
k24 v44324
k25 v84425
k26 v2126
k27 v40327
k28 v96228
k29 v48029
k3 v2653
k30 v1630
k31 v96531
k32 v12332
k33 v84133
k34 v22934
k35 v19635
k36 v92036
k37 v86337
k38 v90038
k39 v8139
k4 v724
k40 v92140
k41 v82841
k42 v29342
k43 v29743
k44 v2044
k45 v38345
k46 v44346
k47 v42147
k48 v86348
k49 v97949
k5 v795
k50 v8750
k51 v12751
k52 v95152
k53 v20153
k54 v84954
k55 v39355
k56 v4456
k57 v87057
k58 v79658
k59 v559
k6 v6336
k60 v34960
k61 v81361
k62 v97062
k63 v47363
k64 v65464
k65 v19965
k66 v67066
k67 v57367
k68 v6268
k69 v56969
k7 v7037
k70 v65570
k71 v98371
k72 v39772
k73 v94873
k74 v28074
k75 v41775
k76 v33076
k77 v72377
k78 v83978
k79 v19379
k8 v1618
k80 v70180
k81 v92681
k82 v32082
k83 v65283
k84 v12684
k85 v16885
k86 v4486
k87 v17087
k88 v3888
k89 v84089
k9 v579
k90 v17690
k91 v38891
k92 v65392
k93 v14593
k94 v86194
k95 v30695
k96 v34596
k97 v53197
k98 v88098
k99 v40799