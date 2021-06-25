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

$json_string = json_encode($_POST);
$file_handle = fopen('test.json', 'w');
fwrite($file_handle, $json_string);
fclose($file_handle);

$data = $_POST['data'];
$data_expanded = explode("\n",$data);
$stmt = $dbh->prepare("INSERT INTO `ComputerInformation` VALUES (?,NOW(),?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
foreach($data_expanded as $d){
	$tmp = explode("<|>",$d);
	if(!empty($tmp[0])){
		$stmt->execute(array($clientid,	//		 ClientID
		 				strtoupper($tmp[0]),		// [0] = ComputerGUID
	 					$tmp[1],		// [1] = Name
						$tmp[2],		// [2] = Manufacturer
						$tmp[3],		// [3] = Model
						$tmp[4],		// [4] = Domain
						$tmp[5],		// [5] = Workgroup
						$tmp[6],		// [6] = IPV4Address
						$tmp[7],		// [7] = IPV6Address
						$tmp[8],		// [8] = MACAddress
						$tmp[9],		// [9] = CPU
						$tmp[10],		// [10] = Cores
						$tmp[11],		// [11] = RAM
						(int) $tmp[12],		// [12] = DriveCapacity
						(int) $tmp[13],		// [13] = DriveSpace
						$tmp[14],		// [14] = OS
						$tmp[15],		// [15] = OSVersion
						$tmp[16],		// [16] = OSArchitecture
						$tmp[17],		// [17] = OSSerialNumber
						$tmp[18],		// [18] = OSInstallDate
						$tmp[19],		// [19] = LastBootUpTime
						$tmp[20]));	// [22] = NumberOfUsers
	}
}

$stmt = null;
$dbh = null;

?>
