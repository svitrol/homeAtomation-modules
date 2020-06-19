  <?php
       if($_POST){
          require_once('Db.php');
          Db::connect('localhost', 'prihlaseni', $_POST['jmeno'], $_POST['heslo']);
          //Db::connect('localhost', 'prihlaseni', 'root','');
          if(isset($_POST['akce'])){
              if($_POST['akce']=="update"){
              $sql= "UPDATE `lidskypocitac` SET ";
              if(isset($_POST['R1'])&&strcmp($_POST['R1'],"N")) $sql=$sql."`R1`=" .$_POST['R1'].", ";
              if(isset($_POST['R2'])&&strcmp($_POST['R2'],"N")) $sql=$sql."`R2`=" .$_POST['R2'].", ";
              if(isset($_POST['R3'])&&strcmp($_POST['R3'],"N")) $sql=$sql."`R3`=" .$_POST['R3'].", ";
              if(isset($_POST['R4'])&&strcmp($_POST['R4'],"N")) $sql=$sql."`R4`=" .$_POST['R4'].", ";
              if(isset($_POST['R5'])&&strcmp($_POST['R5'],"N")) $sql=$sql."`R5`=" .$_POST['R5'].", ";
              if(isset($_POST['R6'])&&strcmp($_POST['R6'],"N")) $sql=$sql."`R6`=" .$_POST['R6'].", ";
              if(isset($_POST['R7'])&&strcmp($_POST['R7'],"N")) $sql=$sql."`R7`=" .$_POST['R7'].", ";
              if(isset($_POST['R8'])&&strcmp($_POST['R8'],"N")) $sql=$sql."`R8`=" .$_POST['R8'].", ";
              $sql=$sql." `casUpravy`=CURRENT_TIMESTAMP WHERE `id`=1;";
              Db::query($sql);              
              }
              else{
                  if($_POST['akce']=="stav"){
                    $sql="SELECT * FROM `lidskypocitac` WHERE `id`=1; ";
                    $vysledek=Db::queryAll($sql);
                    $stavy="";
                    $stavy=$stavy."R1:".$vysledek[0]['R1'].";";
                    $stavy=$stavy."R2:".$vysledek[0]['R2'].";";
                    $stavy=$stavy."R3:".$vysledek[0]['R3'].";";
                    $stavy=$stavy."R4:".$vysledek[0]['R4'].";";
                    $stavy=$stavy."R5:".$vysledek[0]['R5'].";";
                    $stavy=$stavy."R6:".$vysledek[0]['R6'].";";
                    $stavy=$stavy."R7:".$vysledek[0]['R7'].";";
                    $stavy=$stavy."R8:".$vysledek[0]['R8'];
                    echo($stavy);
                    //print_r($vysledek); 
                   
              
                }
              }
          
          }    

       }
  ?>