<?php

// PAGE FOR RECORDING CHECKIN.  CLIENTS WILL RECEIVE STATUSCODE AND ACT ON THAT IF != 1

// Simpler way of making sure all no-cache headers get sent
// and understood by all browsers, including IE.
session_cache_limiter('nocache');
header('Expires: ' . gmdate('r', 0));
header('Content-type: application/json');

require_once('../shared/db.php');

//
// WHAT TO DO IF $_GET['id'] ISN'T SET
//

var_dump(get_defined_vars());
//var_dump($_POST);
$clientguid = $_GET['id'];

if(!preg_match("/[0-9A-Z]{8}-([0-9A-Z]{4}-){3}[0-9A-Z]{12}/",$clientguid)){
	echo json_encode(array('error'=>'invalid guid'));die();
}

// VERIFY CHECKING IN CLIENT EXISTS,
// IF SO RECORD LASTCHECKIN=NOW(),
// ELSE INSERT CLIENT
$stmt = $dbh->prepare("SELECT `ClientID` FROM `Clients` WHERE `ClientGUID`=?");
$stmt->execute(array($clientguid));
$row = $stmt->fetch(PDO::FETCH_OBJ);
if($stmt->rowCount() === 1){
	// WE FOUND A MATCH
	$stmt2 = $dbh->prepare("UPDATE `Clients` SET LastCheckIn=NOW() WHERE `ClientGUID`=?");
	$stmt2->execute(array($clientguid));

	if(empty($_POST['service'])){
		echo json_encode(array("result" => 1, "status" => 1));
	}

	//
	// WHAT TO DO IF FAILED UPDATE?
	//
}else{
	// NO MATCH FOUND, INSERT NEW CLIENT
	$stmt2 = $dbh->prepare("INSERT INTO `Clients` VALUES (NULL,?,NOW(),NOW())");
	$stmt2->execute(array($clientguid));

	if(!isset($_POST['service'])){
		echo json_encode(array("result" => 0, "status" => -1));
	}
	//
	// WHAT TO DO IF FAILED INSERT?
	//
}
// VERIFY SERVICE SPECIFIED,
// IF SO UPDATE CLIENTSERVICE'S LASTCHECKIN=NOW(),
// ELSE INSERT CLIENTSERVICE
if(isset($_POST['service']) && !empty($_POST['service'])){
	$service = $_POST['service'];

	$stmt = $dbh->prepare("	SELECT	c.`ClientID`,
									s.`ServiceID`
							FROM	`Clients` c JOIN `ClientServices` cs
							ON		c.`ClientID` = cs.`ClientID`
									JOIN `Services` s
							ON		cs.`ServiceID` = s.`ServiceID`
							WHERE	c.`ClientGUID`=?
							AND		s.`Name`=?");
	$stmt->execute(array($clientguid, $service));
	$rowids = $stmt->fetch(PDO::FETCH_OBJ);

	if($stmt->rowCount() >= 1){
		// WE FOUND A MATCH

		$stmt2 = $dbh->prepare("UPDATE `ClientServices` SET LastCheckIn=NOW() WHERE `ClientID`=? AND `ServiceID`=?");
		$stmt2->execute(array($rowids->ClientID, $rowids->ServiceID));
		echo json_encode(array("result" => 2, "status" => 1));
	}else{
		// NO MATCH FOUND, INSERT NEW CLIENTSERVICE:
		// FIRST, GET CLIENTID AND SERVICEID
		$SQLclntid = $dbh->prepare("SELECT `ClientID` FROM `Clients` WHERE `ClientGUID`=?");
		$SQLclntid->execute(array($clientguid));
		$clntid = $SQLclntid->fetch(PDO::FETCH_OBJ);

		$SQLsvcid = $dbh->prepare("SELECT `ServiceID` FROM `Services` WHERE `Name`=?");
		$SQLsvcid->execute(array($service));
		$svcid = $SQLsvcid->fetch(PDO::FETCH_OBJ);

		// NEXT, GET DEFAULT VERSION FOR SERVICE SPECIFIED
		$settingname = "version" . $service;
		$stmtver = $dbh->prepare("SELECT `Value` FROM `DefaultSettings` WHERE `Name`=?");
		$stmtver->execute(array($settingname));
		$rowver = $stmtver->fetch(PDO::FETCH_OBJ);
		echo $rowver->Value;

		// INSERT NEW CLIENTSERVICE WITH SPECIFIED DEFAULT VERSION
		$stmt2 = $dbh->prepare("INSERT INTO `ClientServices` VALUES (?,?,?,?,NOW())");
		$stmt2->execute(array($clntid->ClientID, $svcid->ServiceID, 1, $rowver->Value));
		echo json_encode(array("result" => 0, "status" => 1));

		//
		// WHAT TO DO IF FAILED INSERT?
		//
	}
}

$stmt = null;
$dbh = null;

?>
