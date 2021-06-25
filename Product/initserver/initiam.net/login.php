<?php
   ob_start();
   session_start();
   
   
   
   
?>

<?
   // error_reporting(E_ALL);
   // ini_set("display_errors", 1);
?>

<html lang = "en">
   
	<head>
		<title>InITIAM</title>
		<link href = "css/bootstrap.min.css" rel = "stylesheet">
      
		<style>
			body {
				padding-top: 40px;
				padding-bottom: 40px;
				background-color: #FFFFFF;
			 }
			 .logo {
				height: 50%;
				display: block;
				margin-left: auto;
				margin-right: auto;
			 }
			 
			 .form-signin {
				max-width: 330px;
				padding: 15px;
				margin: 0 auto;
				color: #f2d50c;
			 }
			 
			 .form-signin .form-signin-heading,
			 .form-signin .checkbox {
				margin-bottom: 10px;
			 }
			 
			 .form-signin .checkbox {
				font-weight: normal;
			 }
			 
			 .form-signin .form-control {
				position: relative;
				height: auto;
				-webkit-box-sizing: border-box;
				-moz-box-sizing: border-box;
				box-sizing: border-box;
				padding: 10px;
				font-size: 16px;
			 }
			 
			 .form-signin .form-control:focus {
				z-index: 2;
			 }
			 
			 .form-signin input[type="text"] {
				margin-bottom: -1px;
				border-bottom-right-radius: 0;
				border-bottom-left-radius: 0;
				border-color:#f2d50c;
			 }
			 
			 .form-signin input[type="password"] {
				margin-bottom: 10px;
				border-top-left-radius: 0;
				border-top-right-radius: 0;
				border-color:#f2d50c;
			 }
			 
		  </style>
		  
	   </head>
		
	   <body>
			<img src="bin/resources/images/initlogo.png" alt="InITIAM" class="logo" />
		  
		  <div class = "container form-signin">
			 
			 <?php
				require_once('srv/shared/userdb.php');
				$msg = '';
				
				if (isset($_POST['login'])) {
					$username = $_POST['username'];
					$password = $_POST['password'];
					
					$stmt = $dbh->prepare("SELECT * FROM Users WHERE Username=:username");
					$stmt->bindParam("username", $username, PDO::PARAM_STR);
					$stmt->execute();
					
					$row = $stmt->fetch(PDO::FETCH_ASSOC);
				  
					if (!$row) {
						echo 'Username password combination is wrong!';
					} else {
						if (password_verify($password, $row['Password'])) {
							// Create sessions to know the user is logged in
						  
							$_SESSION['user_id'] = $row['UserID'];
							$_SESSION['loggedin'] = true;
							echo 'Successfully logged in!';
							header("location: index.php");
						} else {
							echo 'Username password asdfcombination is wrong!';
						}
					} 
				} else {
					echo 'Must enter Username and Password!';
				}
			 ?>
		  </div> <!-- /container -->
		  
		  <div class = "container">
		  
			 <form class = "form-signin" role = "form" 
				action = "<?php echo htmlspecialchars($_SERVER['PHP_SELF']); 
				?>" method = "post">
				<h4 class = "form-signin-heading"><?php echo $msg; ?></h4>
				<input type = "text" class = "form-control" 
				   name = "username" placeholder = "username" 
				   required autofocus></br>
				<input type = "password" class = "form-control"
				   name = "password" placeholder = "password" required>
				</br>
				<button class = "btn btn-lg btn-primary btn-block" type = "submit" 
				   name = "login">Login</button>
			 </form>
				
			 Click here to clean <a href = "logout.php" tite = "Logout">Session.
			 
		  </div> 
		  
	   </body>
</html>