<?php

require_once('../shared/db.php');

$filetag = $_GET['filetag'];

if(isset($filetag)){
	//$file = preg_replace('/[^0-9a-zA-Z]+/i','',$_GET['file']);
	//$filename = './tmplink/'.$file.'.zip';

	$stmt = $dbh->prepare("	SELECT	`Filepath`
							FROM	ServiceReleases sr
							JOIN	ReleaseRequests rr
									ON sr.`ServiceReleaseID` = rr.`ServiceReleaseID`
							WHERE	rr.`Filetag`=?
							AND		rr.`DateCompleted` IS NULL");
	$stmt->execute(array($filetag));
	$row = $stmt->fetch(PDO::FETCH_OBJ);
	
	if($stmt->rowCount() === 1){
		
		$filepath = $row->Filepath . "/package.zip";

		if(file_exists($filepath)){
			ignore_user_abort(true);
			
			header('Content-Description: File Transfer');
			header('Content-Type: application/octet-stream');// ...? application/octet-stream  application/download
			header('Content-Disposition: attachment; filename="'.basename($filepath).'"');
			header('Content-Length: ' . filesize($filepath));//filesize("sub/$doc_file")
			header('Content-Transfer-Encoding: binary');
			flush();
			//readfile($filename);
			// http://stackoverflow.com/a/8771313
			$fp=fopen("$filepath","rb");
			while(!feof($fp))
			{
				print(fread($fp,1024*8));
				flush();
				ob_flush();
				if( connection_aborted() )
				{
					fclose($fp);
					die();
				}
			}
			fclose($fp);
			
			$stmt = $dbh->prepare("	UPDATE	ReleaseRequests
									SET		`DateCompleted`=NOW()
									WHERE	`Filetag`=?");
			$stmt->execute(array($filetag));
			
			//header("Location: https://initiam.net"); /* Redirect browser */
			die();

		}else{
			die('error');
		}
	}
	
}
?>
