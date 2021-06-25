<?php
//phpinfo();
// Initialize session
session_start();

// Check if the user is logged in, if not then redirect to login page
if(!isset($_SESSION['loggedin']) || $_SESSION['loggedin'] !== true){
	header("location: login.php");
	exit;


}
?>

<html lang = "en">
   
	<head>
		<title>InITIAM</title>
		<link href = "css/bootstrap.min.css" rel = "stylesheet">
		
		<style>
			.logo {
				width: 16%;
			}
			
			/* The side navigation menu */
			.sidebar {
			  height: 100%; /* 100% Full-height */
			  width: 0; /* 0 width - change this with JavaScript */
			  position: fixed; /* Stay in place */
			  z-index: 1; /* Stay on top */
			  top: 0; /* Stay at the top */
			  left: 0;
			  background-color: #111; /* Black*/
			  overflow-x: hidden; /* Disable horizontal scroll */
			  padding-top: 60px; /* Place content 60px from the top */
			  transition: 0.5s; /* 0.5 second transition effect to slide in the sidebar */
			}

			/* The navigation menu links */
			.sidebar a {
			  padding: 8px 8px 8px 32px;
			  text-decoration: none;
			  font-size: 25px;
			  color: #818181;
			  display: block;
			  transition: 0.3s;
			}

			/* When you mouse over the navigation links, change their color */
			.sidebar a:hover {
			  color: #f1f1f1;
			}

			/* Position and style the close button (top right corner) */
			.sidebar .closebtn {
			  position: absolute;
			  top: 0;
			  right: 25px;
			  font-size: 36px;
			  margin-left: 50px;
			}

			/* The button used to open the sidebar */
			.openbtn {
			  text-align: center;
			  line-height: 65px;
			  font-size: 20px;
			  cursor: pointer;
			  background-color: #111;
			  color: white;
			  padding: 10px 15px;
			  border: none;
			  float: left;
			  display: flex;
			  flex-flow: column wrap;
			}

			.openbtn:hover {
			  background-color: #444;
			}

			/* Style page content - use this if you want to push the page content to the right when you open the side navigation */
			#main {
			  transition: margin-left .5s; /* If you want a transition effect */
			  padding: 20px;
			}
			
			#header {
				justify-content: center;
				align-items: center;
				border: 2px;
				border-style: solid;
				border-color: #111;
				display: flex;
				position: relative;
			}
			
			#dashboard {
				margin-top: 20px;
			}

			/* On smaller screens, where height is less than 450px, change the style of the sidebar (less padding and a smaller font size) */
			@media screen and (max-height: 450px) {
			  .sidebar {padding-top: 15px;}
			  .sidebar a {font-size: 18px;}
			}
		</style>
		
		<script>
			/* Set the width of the side navigation to 250px and the left margin of the page content to 250px */
			function openNav() {
			  document.getElementById("mySidebar").style.width = "250px";
			  document.getElementById("main").style.marginLeft = "250px";
			}

			/* Set the width of the side navigation to 0 and the left margin of the page content to 0 */
			function closeNav() {
			  document.getElementById("mySidebar").style.width = "0";
			  document.getElementById("main").style.marginLeft = "0";
			}
			
			function logout() {
				window.location.replace("logout.php");
			}
			
			function calculateProjection() {
				
			}
		</script>
		
		<script>
			<?php
							
				require_once('../srv/shared/webdb.php');
				
				$computernamessql =	"
									SELECT Name FROM ComputerInformation GROUP BY Name, ComputerGUID;
									";
				
				$diskusagesql =		"
									SELECT	UNIX_TIMESTAMP(s.EntryDate) * 1000 AS x,
											(s.DriveSpace/(1024*1024*1024)) AS y
									FROM	(			
										SELECT	Name,
												DriveSpace,
												DriveCapacity,
												EntryDate
										FROM	ComputerInformation
										WHERE	Name LIKE ?
										AND		EntryDate BETWEEN NOW() - INTERVAL 30 DAY AND NOW()
										ORDER BY EntryDate DESC
									) s
									ORDER BY EntryDate ASC;
									";
				
				$computernamesstmt = $dbh->prepare($computernamessql);
				$computernamesrslt = $computernamesstmt->execute();					
				
				$firstcomputername = '';
				$nameoptions = '';
				$namenum = 0;
				while($computernamerow = $computernamesstmt->fetch(PDO::FETCH_ASSOC))
				{
					if($namenum === 0){
						$firstcomputername = $computernamerow['Name'];
					}
					$nameoptions .= '<option value = "'.$computernamerow['Name'].'">'.$computernamerow['Name'].'</option>';
					$namenum = $namenum + 1;
				}

				$diskusagestmt = $dbh->prepare($diskusagesql);
				$diskusagerslt = $diskusagestmt->execute(array($firstcomputername));
				$diskusagerows = $diskusagestmt->fetchAll(PDO::FETCH_OBJ);
				
				
				//$computernamesrows = $computernamesstmt->fetchAll(PDO::FETCH_ASSOC);
				
			?>

			window.onload = function () {
				var chart = new CanvasJS.Chart("chartContainer", {
					animationEnabled: true,
					zoomEnabled: true,
					title: {
						text: <?php if(	isset($_POST['inName']) && 
										isset($_POST['inUnit']) && 
										isset($_POST['inInterval'])){
											echo '"Projected Disk Usage for '.$_POST['inName'].'"';
										} else {
											echo '"Current Disk Usage for '.$firstcomputername.'"';
										}
								?>
					},
					axisX: {
						title: "Time"
					},
					axisY: {
						title: "Disk Space Remaining",
						suffix: "GB"
					},
					data: [{
						type: "line",
						name: "CPU Utilization",
						connectNullData: true,
						//nullDataLineDashType: "solid",
						xValueType: "dateTime",
						xValueFormatString: "MM-DD-YY HH:mm:ss",
						yValueFormatString: "# 'GB'",
						dataPoints: <?php 
										if(isset($_POST['inName']) && isset($_POST['inUnit']) && isset($_POST['inInterval'])){
											$computername = $_POST['inName'];
											$unit = $_POST['inUnit'];
											$interval = $_POST['inInterval'];
											
											if($interval === 'YEAR'){
												$unit = $unit * 31556926;
											}
											if($interval === 'MONTH'){
												$unit = $unit * 2629743;
											}
											if($interval === 'DAY'){
												$unit = $unit * 86400;
											}
											if($interval === 'HOUR'){
												$unit = $unit * 3600;
											}
											$deltagbpersecsql =	"
														SELECT	AVG(gbchange / tmchange) + (AVG(gbchange / tmchange) * ?) AS DeltaGBperSec INTO @DeltaGBperSec
														FROM	(
															SELECT	s.EntryDate AS x,
																	UNIX_TIMESTAMP(s.EntryDate) AS UnixTime,
																	(s.DriveSpace/(1024*1024*1024)) AS y,
																	if(@last_entrygb = 0, 0, (s.DriveSpace/(1024*1024*1024)) - @last_entrygb) AS gbchange,
																	if(@last_entrytm = 0, 0, UNIX_TIMESTAMP(s.EntryDate) - @last_entrytm) AS tmchange,
																	@last_entrygb := (s.DriveSpace/(1024*1024*1024)),
																	@last_entrytm := UNIX_TIMESTAMP(s.EntryDate)
															FROM	(
																select 	@last_entrygb := 0,
																		@last_entrytm := 0
															) z,
																	(			
																SELECT	Name,
																		DriveSpace,
																		DriveCapacity,
																		EntryDate
																FROM	ComputerInformation
																WHERE	Name LIKE ?
																AND		EntryDate BETWEEN NOW() - INTERVAL ? SECOND AND NOW()
																ORDER BY EntryDate DESC
															) s
															ORDER BY EntryDate ASC
														) a;
														";
											
											$deltagbpersecstmt = $dbh->prepare($deltagbpersecsql);
											$multiplier = 0;
											if(isset($_POST['inChangePercent'])){
												$multiplier = $_POST['inChangePercent'] / 100;
												
											}
											$deltagbpersecrslt = $deltagbpersecstmt->execute(array($multiplier, $computername, $unit));
											
											
											
											$projectionsql =	"
																SELECT	Name,
																		DriveSpace,
																		DriveCapacity,
																		EntryDate,
																		@DeltaGBperSec AS DeltaGBperSec,
																		((-1 * CAST(DriveSpace AS SIGNED)) / (1024*1024*1024) ) / @DeltaGBperSec AS SecTilFull,
																		2147483647 AS LastUnixTime,
																		CASE
																			WHEN (UNIX_TIMESTAMP(EntryDate) + (((-1 * CAST(DriveSpace AS SIGNED)) / (1024*1024*1024) ) / @DeltaGBperSec)) < 2147483647 THEN FROM_UNIXTIME((UNIX_TIMESTAMP(EntryDate) + (((-1 * CAST(DriveSpace AS SIGNED)) / (1024*1024*1024) ) / @DeltaGBperSec)))
																			ELSE FROM_UNIXTIME(2147483647)#STR_TO_DATE('2038-01-19 03:14:07', '%Y-%m-%d %H:%i:%s')
																		END AS DateWhenFull,
																		#FROM_UNIXTIME((UNIX_TIMESTAMP(EntryDate) + (DriveSpace / @DeltaGBperSec))) AS DateWhenFull,
																		((((-1 * CAST(DriveSpace AS SIGNED)) / (1024*1024*1024) ) / @DeltaGBperSec) DIV 31556926) AS Years,
																		(((((-1 * CAST(DriveSpace AS SIGNED)) / (1024*1024*1024) ) / @DeltaGBperSec) % 31556926) DIV 2629743) AS Months,
																		((((((-1 * CAST(DriveSpace AS SIGNED)) / (1024*1024*1024) ) / @DeltaGBperSec) % 31556926) % 2629743) DIV 604800) AS Weeks,
																		(((((((-1 * CAST(DriveSpace AS SIGNED)) / (1024*1024*1024) ) / @DeltaGBperSec) % 31556926) % 2629743) % 604800) DIV 86400) AS Days,
																		((((((((-1 * CAST(DriveSpace AS SIGNED)) / (1024*1024*1024) ) / @DeltaGBperSec) % 31556926) % 2629743) % 604800) % 86400) DIV 3600) AS Hours,
																		(((((((((-1 * CAST(DriveSpace AS SIGNED)) / (1024*1024*1024) ) / @DeltaGBperSec) % 31556926) % 2629743) % 604800) % 86400) % 3600) DIV 60) AS Minutes,
																		((((((((((-1 * CAST(DriveSpace AS SIGNED)) / (1024*1024*1024) ) / @DeltaGBperSec) % 31556926) % 2629743) % 604800) % 86400) % 3600) % 60) DIV 1) AS Seconds
																INTO	@name,
																		@drivespace,
																		@drivecapacity,
																		@entrydate,
																		@DeltaGBperSec,
																		@sectilfull,
																		@lastunixtime,
																		@datewhenfull,
																		@years,
																		@months,
																		@weeks,
																		@days,
																		@hours,
																		@minutes,
																		@seconds
																FROM	ComputerInformation
																WHERE	Name LIKE ?
																AND		EntryDate BETWEEN NOW() - INTERVAL ? HOUR AND NOW()
																ORDER BY EntryDate DESC LIMIT 1;
																";
											
											$projectionstmt = $dbh->prepare($projectionsql);
											$projectionrslt = $projectionstmt->execute(array($computername, $unit));
											
											// Temp Test @DeltaGBperSec
											$teststmt = $dbh->prepare("SELECT	@name AS name,
																				@drivespace AS drivespace,
																				@drivecapacity AS drivecapacity,
																				@entrydate AS entrydate,
																				@DeltaGBperSec AS DeltaGBperSec,
																				@sectilfull AS sectilfull,
																				@lastunixtime AS lastunixtime,
																				@datewhenfull AS datewhenfull,
																				@years AS years,
																				@months AS months,
																				@weeks AS weeks,
																				@days AS days,
																				@hours AS hours,
																				@minutes AS minutes,
																				@seconds AS seconds;
																				");
											$testrslt = $teststmt->execute();
											$testrow = $teststmt->fetch(PDO::FETCH_ASSOC);
											
											$projecteddisksql =	"
																(
																SELECT	UNIX_TIMESTAMP(s.EntryDate) * 1000 AS x,
																		(s.DriveSpace/(1024*1024*1024)) AS y
																FROM	(			
																	SELECT	Name,
																			DriveSpace,
																			DriveCapacity,
																			EntryDate
																	FROM	ComputerInformation
																	WHERE	Name LIKE ?
																	AND		EntryDate BETWEEN NOW() - INTERVAL ? SECOND AND NOW()
																	ORDER BY EntryDate DESC
																) s
																ORDER BY EntryDate ASC
																)
																UNION ALL
																(
																SELECT	ROUND(IF(
																			(UNIX_TIMESTAMP(@entrydate) + @sectilfull) <= @lastunixtime, 
																			(UNIX_TIMESTAMP(@entrydate) + @sectilfull) - 1, 
																			@lastunixtime - 1
																		) * 1000, 0) AS x,
																		NULL AS y
																)
																UNION ALL
																(
																SELECT	ROUND(IF(
																			(UNIX_TIMESTAMP(@entrydate) + @sectilfull) <= @lastunixtime, 
																			(UNIX_TIMESTAMP(@entrydate) + @sectilfull), 
																			@lastunixtime
																		) * 1000, 0) AS x,
																		IF(
																			(UNIX_TIMESTAMP(@entrydate) + @sectilfull) <= @lastunixtime, 
																			0, #Yes
																			(@lastunixtime - UNIX_TIMESTAMP(@entrydate)) * @DeltaGBperSec
																		) AS y #No X * Delta = DriveSpace -- X = lastunixtime - entrydate
																);# Watch query carefully and test.  If it doesn't work, set y = 0
																";
																
											$projecteddiskstmt = $dbh->prepare($projecteddisksql);
											$projecteddiskrslt = $projecteddiskstmt->execute(array($computername, $unit));
											$projecteddiskrows = $projecteddiskstmt->fetchAll(PDO::FETCH_OBJ);
											echo json_encode($projecteddiskrows, JSON_NUMERIC_CHECK);
										} else {
											echo json_encode($diskusagerows, JSON_NUMERIC_CHECK); 
										}
									?>
					}]
				});
				chart.render();
			}
		</script>
		
	</head>
	<body>
		

		<div id="mySidebar" class="sidebar">
			<a href="javascript:void(0)" class="closebtn" onclick="closeNav()">&times;</a>
			<a href="https://initiam.net">Dashboard</a>
			<a href="https://initiam.net/forecasting">Forecasting</a>
			<a href="https://initiam.net/adminer" target="_blank">DB Admin</a>
			<a href="#">Contact</a>
		</div>

		<div id="main">
			
			<div id="header">
				<button class="openbtn" id="btnOpen" onclick="openNav()">&#9776;</button>
				<a href="https://initiam.net">
					<img src="../bin/resources/images/initlogo_hz.png" alt="InITIAM" class="logo" />
				</a>
				<div style="margin-left: auto;margin-right: 0;display: flex;">
					<button class="openbtn" id="btnLogout" onclick="logout()">Logout</button>
				</div>
			</div>
			
			<div id="projectionform" styel="margin=20px;">
				<h2>Project Time Until Disk Capacity Reached</h2>
				<form class="formProjection" role = "form" method="post">
					<?php
						if(isset($_POST['inName']) && isset($_POST['inUnit']) && isset($_POST['inInterval'])){
							echo '<label for="inName">Computer Name:</label>';
							echo '<select name="inName" value="'.$firstcomputername.'">';
								echo $nameoptions;
							echo '</select>';
							echo '<label for="inUnit">Unit:</label>';
							echo '<input type="text" id="inUnit" name="inUnit" placeholder="30" size="5" value="'.$_POST['inUnit'].'">';
							echo '<label for="inInterval">Interval:</label>';
							echo '<select name="inInterval" value="'.$_POST['inInterval'].'">';
								echo '<option value="HOUR">Hour(s)</option>';
								echo '<option value="DAY">Day(s)</option>';
								echo '<option value="MONTH">Month(s)</option>';
								echo '<option value="YEAR">Year(s)</option>';
							echo '</select>';
							echo '<label for="inChangePercent">Usage Incr/Decr (%):</label>';
							echo '<input type="text" id="inChangePercent" name="inChangePercent" size="20" value="'.$_POST['inChangePercent'].'">';
							echo '<button class = "btn btn-lg btn-primary btn-block" type = "submit" name="project" id="project">Project</button>';
						} else {
							echo '<label for="inName">Computer Name:</label>';
							echo '<select name="inName">';
								echo $nameoptions;
							echo '</select>';
							echo '<label for="inUnit">Unit:</label>';
							echo '<input type="text" id="inUnit" name="inUnit" placeholder="30" size="5" value="30">';
							echo '<label for="inInterval">Interval:</label>';
							echo '<select name="inInterval">';
								echo '<option value="HOUR">Hour(s)</option>';
								echo '<option value="DAY" selected>Day(s)</option>';
								echo '<option value="MONTH">Month(s)</option>';
								echo '<option value="YEAR">Year(s)</option>';
							echo '</select>';
							echo '<label for="inChangePercent">Usage Incr/Decr (%):</label>';
							echo '<input type="text" id="inChangePercent" name="inChangePercent" size="20" value="0">';
							echo '<button class = "btn btn-lg btn-primary btn-block" type = "submit" name="project" id="project">Project</button>';
						}
					?>
				</form>
			</div>
			
			<div id="dashboard">
				<script src="../canvasjs/canvasjs.min.js"></script>
				<div id="chartContainer" style="height: 370px; margin: 0px auto;"></div>
			</div>
			
			<div id="projectionSidebar">
				<p>Note: If final date = 01/19/2038, the projected date is greater than the largest possible Unix Timestamp.  See units below:</p>
				<p>Years: 	<?php 
								if(isset($_POST['inName']) && isset($_POST['inUnit']) && isset($_POST['inInterval'])){
									echo $testrow['years']; 
								}
							?>
				</p>
				<p>Months: 	<?php 
								if(isset($_POST['inName']) && isset($_POST['inUnit']) && isset($_POST['inInterval'])){
									echo $testrow['months']; 
								}
							?>
				</p>
				<p>Days: 	<?php 
								if(isset($_POST['inName']) && isset($_POST['inUnit']) && isset($_POST['inInterval'])){
									echo $testrow['days']; 
								}
							?>
				</p>
				<p>Hours: 	<?php 
								if(isset($_POST['inName']) && isset($_POST['inUnit']) && isset($_POST['inInterval'])){
									echo $testrow['hours']; 
								}
							?>
				</p>
				<p>Minutes: <?php 
								if(isset($_POST['inName']) && isset($_POST['inUnit']) && isset($_POST['inInterval'])){
									echo $testrow['minutes']; 
								}
							?>
				</p>
				<p>Seconds: <?php 
								if(isset($_POST['inName']) && isset($_POST['inUnit']) && isset($_POST['inInterval'])){
									echo $testrow['seconds']; 
								}
							?>
				</p>
			</div>
			
			
		</div>
	</body>
</html>