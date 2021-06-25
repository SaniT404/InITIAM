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
		</script>
		
		<script>
			<?php
							
				require_once('srv/shared/webdb.php');
				
				$disksql = "
						SELECT 	(i.DriveCapacity/(1024*1024*1024)) AS x, 
								(i.DriveCapacity/(1024*1024*1024)) - (i.DriveSpace/(1024*1024*1024)) AS y
						FROM	(
							SELECT 	ComputerGUID, MAX(EntryDate) AS MaxEntryDate
							FROM 	ComputerInformation 
							GROUP BY ComputerGUID
						) c INNER JOIN ComputerInformation i
							ON c.ComputerGUID = i.ComputerGUID AND i.EntryDate = c.MaxEntryDate;
						";

				$diskstmt = $dbh->prepare($disksql);
				$diskrslt = $diskstmt->execute();
				$diskrows = $diskstmt->fetchAll(PDO::FETCH_OBJ);
				
				$softwaresql = "
						SELECT  FLOOR(`number`) AS `y`, `Name` AS `label`
								 
						FROM ( 
							SELECT  `Name`, 
									COUNT(*) AS `number` 
							FROM (
								SELECT  `Name`, 
										`ComputerGUID`, 
										COUNT(*) 
								FROM    `SoftwareInformation` 
								GROUP BY `Name`, `ComputerGUID` 
							) AS `s` 
							GROUP BY `Name` 
							ORDER BY COUNT(*) DESC , `Name` ASC LIMIT 5 
						) AS `n`;
						";

				$softwarestmt = $dbh->prepare($softwaresql);
				$softwarerslt = $softwarestmt->execute();
				$softwarerows = $softwarestmt->fetchAll(PDO::FETCH_OBJ);
				
				$ramsql = "
						SELECT 	COUNT(*) AS y,
								CASE
									WHEN RAMGB < 4 THEN '< 4GB'
									WHEN RAMGB > 32 THEN '> 32GB'
									ELSE CONCAT(RAMGB, 'GB')
								END AS label
						FROM	(
							SELECT 	i.ComputerGuid, ROUND((i.RAM/(1024*1024*1024))) AS RAMGB, c.MaxEntryDate
							FROM	(
								SELECT 	ComputerGUID, MAX(EntryDate) AS MaxEntryDate
								FROM 	ComputerInformation 
								GROUP BY ComputerGUID
							) c INNER JOIN ComputerInformation i
								ON c.ComputerGUID = i.ComputerGUID AND i.EntryDate = c.MaxEntryDate
						) r
						GROUP BY RAMGB
						ORDER BY RAMGB ASC;
						";

				$ramstmt = $dbh->prepare($ramsql);
				$ramrslt = $ramstmt->execute();
				$ramrows = $ramstmt->fetchAll(PDO::FETCH_OBJ);
				//$result = json_encode($rows, JSON_NUMERIC_CHECK);
			?>

			window.onload = function () {
			var diskchart = new CanvasJS.Chart("chartContainer", {
				animationEnabled: true,
				zoomEnabled: true,
				title: {
					text: "Disk Space Usage"
				},
				axisX: {
					title: "Drive Capacity (GB)"
				},
				axisY: {
					title: "Drive Usage (GB)",
					suffix: "GB"
				},
				data: [{
					type: "scatter",
					//toolTipContent: "<b>Area: </b>{x} sq.ft<br/><b>Price: </b>${y}k",
					dataPoints: <?php echo json_encode($diskrows, JSON_NUMERIC_CHECK); ?>
				}]
			});
			diskchart.render();

			var softwarechart = new CanvasJS.Chart("softwareChartContainer", {
				animationEnabled: true,
				theme: "light2", // "light1", "light2", "dark1", "dark2"
				title:{
					text: "Most Installed Software"
				},
				axisY: {
					title: "Assets"
				},
				data: [{        
					type: "column",  
					showInLegend: true, 
					legendMarkerColor: "grey",
					legendText: "Software",
					dataPoints: <?php echo json_encode($softwarerows, JSON_NUMERIC_CHECK); ?>
				}]
			});
			softwarechart.render();



			var ramchart = new CanvasJS.Chart("ramChartContainer", {
				animationEnabled: true,
				title:{
					text: "Number of Computers by RAM",
					horizontalAlign: "left"
				},
				data: [{
					type: "doughnut",
					startAngle: 60,
					//innerRadius: 60,
					indexLabelFontSize: 17,
					indexLabel: "{label} - #percent%",
					toolTipContent: "<b>{label}:</b> {y} (#percent%)",
					dataPoints: <?php echo json_encode($ramrows, JSON_NUMERIC_CHECK); ?>
				}]
			});
			ramchart.render();



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
					<img src="bin/resources/images/initlogo_hz.png" alt="InITIAM" class="logo" />
				</a>
				<div style="margin-left: auto;margin-right: 0;display: flex;">
					<button class="openbtn" id="btnLogout" onclick="logout()">Logout</button>
				</div>
			</div>
			
			<div id="dashboard">
				<script src="canvasjs/canvasjs.min.js"></script>
				<div id="chartContainer" style="height: 370px; width: 33%; margin: 0px auto;float: left;"></div>
				<div id="ramChartContainer" style="height: 370px; width: 33%; margin: 0px auto;float: left;"></div>
				<div id="softwareChartContainer" style="height: 370px; width: 33%; margin: 0px auto;"></div>
			</div>
			
			
			
		</div>
	</body>
</html>