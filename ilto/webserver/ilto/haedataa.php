<?php
    header('Content-Type: application/json');
//    header('ini_set("display_errors", "On")');

    $mng = new MongoDB\Driver\Manager("mongodb://localhost:27017");

    $alku = intval($_POST['alku']);
    $loppu = intval($_POST['loppu']);
    $limitti = intval($_POST['limitti']);

    $filter = ['timestamp' => ['$gte' => $alku, '$lte' => $loppu]]; //1482175827]];
    $options = ['sort' => ['timestamp' => -1], 'limit' => $limitti];

    $query = new MongoDB\Driver\Query($filter,  $options);
    $rows = $mng->executeQuery("ilto.data", $query);

    $resp = array();
    $resp["alku"] = $alku;
    $resp["loppu"] = $loppu;
    $resp["limitti"] = $limitti;

    foreach ($rows as $row) {
//        echo "$row->timestamp <br>";
        $temp = json_encode($row, true);
        array_push($resp, $temp);
    }
//    print_r($resp);
    echo json_encode($resp);
?>
<?php // echo phpinfo(); ?>

