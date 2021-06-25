<?php

// PAGE FOR CHECKING CLIENT VERSION

// Simpler way of making sure all no-cache headers get sent
// and understood by all browsers, including IE.
session_cache_limiter('nocache');
header('Expires: ' . gmdate('r', 0));
header('Content-type: application/json');

require_once('../../shared/db.php');

//
// WHAT TO DO IF $_GET['id'] ISN'T SET
//

$clientguid = $_GET['id'];

$stmt = $dbh->prepare("SELECT `ClientID` FROM `Clients` WHERE `ClientGUID`=?");
$stmt->execute(array($clientguid));
$row = $stmt->fetch(PDO::FETCH_ASSOC);

$clientid = $row['ClientID'];

echo json_encode(array("success"=>"1"));

$json_string = json_encode($_POST['data']);
$file_handle = fopen('test.json', 'w');
fwrite($file_handle, $_POST['data']);//
fclose($file_handle);

$data = $_POST['data'];
$data_expanded = array_filter(explode("\n",$data)); // array_filter will remove any blank new lines (anything that evals to false)
$stmt = $dbh->prepare("INSERT INTO `SoftwareInformation` VALUES (?,NOW(),?,?,?,?,?,?)");
foreach($data_expanded as $d){
	$tmplode = explode("<|>",$d);
	if(is_array($tmplode) && $tmplode[1] != "NULL"){
		$tmp = array_map(function($value) { return (($value === "NULL") ? NULL : $value); }, $tmplode); // array_map should walk through $tmplode
		$stmt->execute(array($clientid,				// ClientID
							  strtoupper($tmp[0]),	// [0] = ComputerGUID
							  $tmp[1],				// [1] = Name
							  $tmp[2],				// [2] = Vendor
							  $tmp[3],				// [3] = Version
							  $tmp[4],				// [4] = InstallDate
							  $tmp[5]));			// [22] = InstallState
	}
}

$stmt = null;
$dbh = null;

?>
