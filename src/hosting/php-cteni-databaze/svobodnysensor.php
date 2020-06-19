  <?php
       if($_POST){
          require_once('Db.php');
          Db::connect('localhost', 'prihlaseni', $_POST['jmeno'], $_POST['heslo']);
          $coChce=$_POST['hodnota'];
          $zacatek="SELECT `id`,`$coChce`,`cas` FROM `svetloMoje` WHERE `cas` BETWEEN '";
          $od = new DateTime($_POST['od']);
          $do = new DateTime($_POST['do']);
          $rozdil=date_diff( $do,$od, TRUE );
          $dny= $rozdil->format('%a');
          if($dny>3){
                  if($dny<=7){
                      $odPocatek=new DateTime($_POST['od']);
                      $ohodkuVetsiPocatek=new DateTime($_POST['od']);
                      $ohodkuVetsiPocatek->modify( '+1 hour' );
                      
                        $sql = $zacatek.$odPocatek->format('Y-m-d H:i:s')."' AND '".$ohodkuVetsiPocatek->format('Y-m-d H:i:s')."' ORDER BY `".$coChce."` LIMIT 1 ";
                        $pole=Db::queryAll($sql);
                        $sql = $zacatek.$odPocatek->format('Y-m-d H:i:s')."' AND '".$ohodkuVetsiPocatek->format('Y-m-d H:i:s')."' ORDER BY `".$coChce."`DESC LIMIT 1 ";
                        $je_tu_nekdo=Db::queryAll($sql);
                        array_push($pole, $je_tu_nekdo);
                        $ohodkuVetsiPocatek->modify( '+1 hour' );
                        $odPocatek->modify( '+1 hour' );
                    while($ohodkuVetsiPocatek<$do){
                        $sql = $zacatek.$odPocatek->format('Y-m-d H:i:s')."' AND '".$ohodkuVetsiPocatek->format('Y-m-d H:i:s')."' ORDER BY `".$coChce."` LIMIT 1 ";
                        $je_tu_nekdo=Db::queryAll($sql);
                        array_push($pole, $je_tu_nekdo);
                        $sql = $zacatek.$odPocatek->format('Y-m-d H:i:s')."' AND '".$ohodkuVetsiPocatek->format('Y-m-d H:i:s')."' ORDER BY `".$coChce."`DESC LIMIT 1 ";
                        $je_tu_nekdo=Db::queryAll($sql);
                        array_push($pole, $je_tu_nekdo);
                        $ohodkuVetsiPocatek->modify( '+1 hour' );
                        $odPocatek->modify( '+1 hour' );
                    }
                    $sql = $zacatek.$odPocatek->format('Y-m-d H:i:s')."' AND '".$_POST['do']."'  LIMIT 1 ";
                    $je_tu_nekdo=Db::queryAll($sql);
                    array_push($pole, $je_tu_nekdo);
                    $max = sizeof($pole);
                      for($i = 0; $i < $max;$i++)
                      {                       
                          if(isset($pole[$i][0]['id']))echo($pole[$i][0]['id'].",".$pole[$i][0][$coChce].",".$pole[$i][0]['cas'].";");
                      }
                      
                  }
                  else{
                     $odPocatek=new DateTime($_POST['od']);
                      $oDenVetsiPocatek=new DateTime($_POST['od']);
                      $oDenVetsiPocatek->modify( '+1 day' );
                      
                        $sql = $zacatek.$odPocatek->format('Y-m-d H:i:s')."' AND '".$oDenVetsiPocatek->format('Y-m-d H:i:s')."' ORDER BY `".$coChce."` LIMIT 1 ";
                        $pole=Db::queryAll($sql);
                        $sql = $zacatek.$odPocatek->format('Y-m-d H:i:s')."' AND '".$oDenVetsiPocatek->format('Y-m-d H:i:s')."' ORDER BY `".$coChce."`DESC LIMIT 1 ";
                        $je_tu_nekdo=Db::queryAll($sql);
                        array_push($pole, $je_tu_nekdo);
                        $oDenVetsiPocatek->modify( '+1 day' );
                        $odPocatek->modify( '+1 day' );
                    while($oDenVetsiPocatek<$do){
                        $sql = $zacatek.$odPocatek->format('Y-m-d H:i:s')."' AND '".$oDenVetsiPocatek->format('Y-m-d H:i:s')."' ORDER BY `".$coChce."` LIMIT 1 ";
                        $je_tu_nekdo=Db::queryAll($sql);
                        array_push($pole, $je_tu_nekdo);
                        $sql = $zacatek.$odPocatek->format('Y-m-d H:i:s')."' AND '".$oDenVetsiPocatek->format('Y-m-d H:i:s')."' ORDER BY `".$coChce."`DESC LIMIT 1 ";
                        $je_tu_nekdo=Db::queryAll($sql);
                        array_push($pole, $je_tu_nekdo);
                        $oDenVetsiPocatek->modify( '+1 day' );
                        $odPocatek->modify( '+1 day' );
                    }
                    $sql = $zacatek.$odPocatek->format('Y-m-d H:i:s')."' AND '".$_POST['do']."'  LIMIT 1 ";
                    $je_tu_nekdo=Db::queryAll($sql);
                    array_push($pole, $je_tu_nekdo);
                    $max = sizeof($pole);
                      for($i = 0; $i < $max;$i++)
                      {                       
                          if(isset($pole[$i][0]['id']))echo($pole[$i][0]['id'].",".$pole[$i][0][$coChce].",".$pole[$i][0]['cas'].";");
                      }
                  }
            }
              //echo"je toho moc";
          else{
              $sql = $zacatek.$_POST['od']."' AND '".$_POST['do']."'";
              $pole=Db::queryAll($sql);
              $max = sizeof($pole);
              
                      for($i = 0; $i < $max;$i++)
                      {
                          $value=$pole[$i];
                         if(isset($value['id'])) echo($value['id'].",".$value[$coChce].",".$value['cas'].";");
                      }  
          }
       } 
  ?>