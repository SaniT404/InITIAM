<?php
   session_start();
   unset($_SESSION["user_id"]);
   unset($_SESSION["loggedin"]);
   
   
   echo 'You have cleaned session';
   header('Refresh: 2; URL = login.php');
?>