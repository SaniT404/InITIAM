<?php
// PAGE FOR CHECKING CLIENT VERSION

// Simpler way of making sure all no-cache headers get sent
// and understood by all browsers, including IE.
session_cache_limiter('nocache');
header('Expires: ' . gmdate('r', 0));
header('Content-type: application/json');

require_once('../shared/db.php');

//
// WHAT TO DO IF $_GET['id'] ISN'T SET
//
$clientguid = $_GET['id'];

if (!empty($_SERVER['HTTP_CLIENT_IP']))   //check ip from share internet
{
    $ip=$_SERVER['HTTP_CLIENT_IP'];
}
    elseif (!empty($_SERVER['HTTP_X_FORWARDED_FOR']))   //to check ip is pass from proxy
{
    $ip=$_SERVER['HTTP_X_FORWARDED_FOR'];
}
else
{
    $ip=$_SERVER['REMOTE_ADDR'];
}


if(!preg_match("/[0-9A-Z]{8}-([0-9A-Z]{4}-){3}[0-9A-Z]{12}/",$clientguid)){
	echo json_encode(array('error'=>'invalid guid'));
	die();
}

// DID WE RECEIVE SERVICE INFORMATION VIA POST

if(isset($_POST['service']) && !empty($_POST['service'])){
	$service = strtolower($_POST['service']);
	// FIND VERSION INFO AN DATA
	if(isset($_POST['version']) && !empty($_POST['version'])){
		$version = $_POST['version'];
		if(file_exists('/data/sites/initiam.net/srv/release/'.$service.'/'.$version.'/package.zip')){
			$rando = base_convert(time() . ((int) preg_replace('[^0-9]','',$clientguid)) . mt_rand(1000000,9999999), 10, 36);// to convert back just do base_convert(#, 36, 10);
			//$stmt = $dbh->prepare("---SELECT `` FROM `` WHERE `ClientGUID`=? JOIN `Service`=? AND `Version`=?");
			//$stmt->execute(array($clientguid,$service,$version));
			//$row = $stmt->fetch(PDO::FETCH_ASSOC);
			// NEED Service and Version SELECTED above.
			//copy('/data/servers/init.sgcity.org/web/srv/release/'.$service.'/'.$version.'/package.zip','/data/servers/init.sgcity.org/web/srv/files/tmplink/'.$rando.'.zip');
			$srvrelsel = $dbh->prepare("	SELECT	sr.`ServiceReleaseID`
											FROM	ServiceReleases sr
											JOIN	Services s
													ON sr.`ServiceID` = s.`ServiceID`
											WHERE	s.`Name`=?
											AND		sr.`Release`=?");
			$srvrelsel->execute(array($service,$version));
			$row = $srvrelsel->fetch(PDO::FETCH_OBJ);

			// Insert release request
			$relreqins = $dbh->prepare("INSERT INTO ReleaseRequests VALUES(NULL,?,?,?,NULL,NOW(),NULL)");
			$relreqins->execute(array($row->ServiceReleaseID,$rando,$ip));


			//echo json_encode(array('filetag'=>$rando));
			echo json_encode(array('url'=>'https://initiam.net/srv/files/tmplink.php?filetag='.$rando), JSON_UNESCAPED_SLASHES);
			die();
		}
	}else{
		$stmt = $dbh->prepare("	SELECT	`Version`
								FROM	`ClientServices` cs
								JOIN 	Clients c
										ON cs.ClientID = c.ClientID
								JOIN 	Services s
										ON s.ServiceID = cs.ServiceID
								WHERE	`ClientGUID`=?
								AND		s.`Name`=?");
		$stmt->execute(array($clientguid,$service));
		$row = $stmt->fetch(PDO::FETCH_ASSOC);
		if(isset($row['Version'])){
			echo json_encode(array("version"=>$row['Version']));
		}else{
			echo json_encode(array("result"=>0));
		}
		die();
	}
}else{
	// NO POST DATA, SEND THEM JSON
	$stmt = $dbh->prepare("	SELECT		s.`Name`,
										cs.`Version`
							FROM		`Services` s
							LEFT JOIN	ClientServices cs
										ON s.`ServiceID` = cs.`ServiceID`
							WHERE 		cs.ClientID =
											(	SELECT	ClientID
												FROM	Clients
												WHERE	ClientGUID=?)
							OR			cs.ClientID IS NULL");
	$stmt->execute(array($clientguid));
	$rows = $stmt->fetchAll(PDO::FETCH_ASSOC);
	//var_dump($rows);
	$arr = array();
	if(isset($rows)){
		foreach($rows as $row){
			if($row['Version'] == NULL){
				$arr = array_merge($arr, array($row['Name']=>'null'));
			}else{
				$arr = array_merge($arr, array($row['Name']=>$row['Version']));
			}
		}
	}else{
		$arr = array('result'=>0);
	}
	echo json_encode($arr);
	die();
	
}







?>
