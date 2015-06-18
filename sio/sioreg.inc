<?php
mb_internal_encoding('UTF-8');

class SioReg {
	use Form;
	const SIG="sioreg_";
	public static function sig() { return static::SIG; }
	private static $munge="sha2(concat(ifnull(username,id),'_',ifnull(password,id),'_',ifnull(email,id),'_',ifnull(ts,id)),256)";
	private static $v=array();
	private static $use_un=true;

	function __construct($key=NULL) {
		$this->iniForm($key,@static::$v[static::SIG]);
		$this->key=$key;
		$this->table="sio_user";
		if(static::$use_un) {
			$this->setfld('username');
		}
		$this->setfld('emailp');
		$this->setfld('passwordn','','!skip');
		$this->setfld('passwordb','','!skip');
	}

	public function validate() {
		$retval = true;
		if(static::$use_un) {
			if (isset($this->fields['username'][0])) {
				$un=Settings::esc($this->fields['username'][0]);
				$qry="select count(id) as ok from " . $this->table . " where username='" . $un . "'";
				if ($rx = Settings::$sql->query($qry)) {
					if (strcmp($rx->fetch_row()[0],"0") !== 0) {
						$retval = false;
						$this->seterr("username",Dict::get(static::SIG.'errors_username_exists'));
					}
					$rx->close();
				}
				if (isset($this->fields['passwordn'][0]) && ($this->fields['passwordn'][0] === $this->fields['username'][0])) {
					$this->seterr("passwordn",Dict::get(static::SIG.'errors_new_pw_too_un_simple'));
					$retval = false;
				}
			} else {
				$this->seterr("username",Dict::get(static::SIG.'errors_no_username'));
				$retval = false;
			}
		}

		if (isset($this->fields['emailp'][0])) {
			$em=Settings::esc($this->fields['emailp'][0]);
			$qry="select count(id) as ok from " . $this->table . " where email='".$em."' or emailp='" . $em . "'";
			if ($rx = Settings::$sql->query($qry)) {
				if (strcmp($rx->fetch_row()[0],"0") !== 0) {
					$retval = false;
					$this->seterr("emailp",Dict::get(static::SIG.'errors_email_exists'));
				}
				$rx->close();
			}
			if (isset($this->fields['passwordn'][0]) && ($this->fields['passwordn'][0] === $this->fields['emailp'][0])) {
				$this->seterr("passwordn",Dict::get(static::SIG.'errors_new_pw_too_em_simple'));
				$retval = false;
			}
		} else {
			$this->seterr("emailp",Dict::get(static::SIG.'errors_no_email'));
			$retval = false;
		}
		if (isset($this->fields['passwordn'][0]) && isset($this->fields['passwordb'][0])) {
			$pwa=$this->fields['passwordn'][0];
			$pwb=$this->fields['passwordb'][0];
			if ($pwa !== $pwb ) {
				$this->seterr("passwordn",Dict::get(static::SIG.'errors_new_pw_different'));
				$retval = false;
			}
			$err_ar = array();
			$pw_retval = SIO::$utility->pw_validate($pwa,$err_ar);
			foreach($err_ar as $err) {
				$this->seterr("passwordn",Dict::get($err));
			}
			$retval= $retval && $pw_retval;
		} else {
			$this->seterr("passwordn",Dict::get(static::SIG.'errors_no_password'));
			$retval = false;
		}
		$this->valEmail('emailp',Dict::get(static::SIG .'errors_email_badformat'));
		if ($this->valid) {
			$this->valid = $retval;
		}
	}

	public function populate() { 
		$this->vset('emailp');
	}

	public function commit() {
		$unm = NULL;
		$em=$this->fields['emailp'][0]; Settings::esc($em);
		if(static::$use_un) {
			if (isset($this->fields['username'][0])) {
				$uname=$this->fields['username'][0];
				$unm=$uname;
				Settings::esc($uname);
				$extra="username='".$uname."',";
				$qry="select ".static::$munge." as munge,emailp from " . $this->table . " where username='" .$uname. "' and active='xx'";
			}
		} else { //using email
			if (isset($this->fields['emailp'][0])) { //not empty
				$unm=$this->fields['emailp'][0];
				$extra="username='".uniqid("u",true)."',";
				$qry="select ".static::$munge." as munge,emailp from " . $this->table . " where emailp='" .$em . "' and active='xx'";
			}
		}
		$ph=SIOSetPW::enhash($unm,$this->fields['passwordn'][0]);
		$insql="insert into ".$this->table." set ts=null,".$extra."emailp='".$em."',password='".$ph."',active='xx'";
		Settings::$sql->query($insql);
		if ($r=Settings::$sql->query("select last_insert_id()")) {
			$this->newid = intval($r->fetch_row()[0]);
			if ($this->newid !== 0) {
				Settings::$sql->query("insert into sio_userprofile set user=".$this->newid);
			}
		}
		if ($rx = Settings::$sql->query($qry)) {
			while ($f = $rx->fetch_assoc()) {
				$mail_v=new NView(@static::$v[static::SIG."email_body"]);
				$mail_v->set("//*[@data-tr]/@data-tr/preceding-gap()",static::SIG);
				$mail = new PHPMailer();
				$mail->isSendmail();
				$mail->CharSet='utf-8';
				$mail->Encoding='base64';
				$mail->setFrom('no_reply@redsnapper.net', Dict::get(static::SIG .'note_register_title'));
				$mail->addAddress($em,$f['emailp']);
				$mail->Subject = Dict::get(static::SIG.'note_register_title');
				$mail->isHTML(true);
				$url=$_SERVER["SCRIPT_URI"];
				if (strpos($url, '?') !== false) {
					$url .= '&amp;siof=' . $f['munge'];
				} else {
					$url .= '?siof=' . $f['munge'];
				}
				$mail_v->set("//*[@data-xp='ha']/@href",$url);
				$mail->Body = $mail_v->show(false);
				$mail->AltBody=Dict::get(static::SIG.'mesg_see_html_alt');
				$mail->send();
			}
			$rx->close();
		}
		return true;
	}

	public static function conforms($hat=NULL) {
		$retval=false;
		$ha=Settings::esc($hat);
		$query= "select id from sio_user where ".static::$munge."='".$ha."'";
		if ($rs = Settings::$sql->query($query)) {
			if (Settings::rows($rs) == 1) {
				$retval=true;
			}
			$rs->close();
		}
		return $retval;
	}

	public static function pushit($hat=NULL) {
		if(is_null($hat)) {
			$nv=new NView(@static::$v[static::SIG."check_mail"]);
			$nv->set("//*[@data-tr]/@data-tr/preceding-gap()",static::SIG);
		} else {
			$ha=Settings::esc($hat);
			$query= "update sio_user set email=emailp,active='on' where active='xx' and ".static::$munge."='".$ha."'";
			Settings::$sql->query($query);
			$nv = new NView(@static::$v[static::SIG."success"]);
			$nv->set("//*[@data-tr]/@data-tr/preceding-gap()",static::SIG);
		}
		return $nv;
	}

	public static function initialise($use_un=true,$custom_views=array()) {
		static::$use_un=$use_un;
//views array
		static::$v=array(
			static::SIG."email_body"=>static::SIG."mb.ixml",
			static::SIG."success"=>static::SIG."sv.ixml",
			static::SIG."check_mail"=>static::SIG."pv.ixml"
		);

//translations
		$en = array(
			static::SIG ."prompt_email"=>"Email",
			static::SIG ."prompt_password"=>"Password",
			static::SIG ."prompt_retype_password"=>"Retype Password",
			static::SIG ."button_register"=>"Register",
			static::SIG ."note_register_title"=>"Registration Request",
			static::SIG ."note_register_message"=>" It appears that you have chosen to register on the website.",
			static::SIG ."note_register_action_link"=>" Please click here to confirm your registration.",
			static::SIG ."errors_email_badformat"=>" The email format is not recognised.",
			static::SIG ."errors_email_exists"=>" A user with this email already exists on the system. Please use another email.",
			static::SIG ."errors_no_email"=>" You must enter an email address.",
			static::SIG ."errors_no_password"=>" You must enter a password.",
			static::SIG ."errors_new_pw_different"=>" Both passwords must be the same.",
			static::SIG ."mesg_register_check_email"=>"Thank you for registering.  A confirmation email has been sent to your email address for verification.",
			static::SIG ."mesg_see_html_alt"=>"Please see the html alternative of this email.",
			static::SIG ."mesg_success"=>"You have successfully registered."
		);

		$de = array(
			static::SIG .'prompt_email'=> 'E-Mail-Adresse',
			static::SIG .'prompt_password'=> 'Passwort',
			static::SIG .'prompt_retype_password'=> 'Passwort wiederholen',
			static::SIG .'button_register'=> 'Registrieren',
			static::SIG .'note_register_title'=> 'Registrierung',
			static::SIG .'note_register_message'=> 'Sie möchten sich auf unserer Website registrieren.',
			static::SIG .'note_register_action_link'=> 'Bitte bestätigen Sie Ihre Registrierung.',
			static::SIG .'errors_email_badformat'=> 'Das E-Mail-Format kann nicht erkannt werden.',
			static::SIG ."errors_email_exists"=>" Diese  E-Mail-Adresse wird bereits verwendet. Bitte wählen Sie eine andere E-Mail-Adresse.",
			static::SIG .'errors_no_password'=> "Bitte geben Sie Ihr Passwort ein.",
			static::SIG .'errors_new_pw_different'=> "Die beiden Passwörter müssen übereinstimmen.",
			static::SIG .'mesg_register_check_email'=> "Um Ihre Registrierung zu bestätigen, lesen Sie bitte die E-Mail, die Ihnen gerade zugeschickt wurde.",
			static::SIG .'mesg_see_html_alt'=> "Sollte die E-Mail nicht korrekt dargestellt werden, wechseln Sie bitte in das HTML-Format.",
			static::SIG .'mesg_success'=> "Sie wurden erfolgreich registriert."
		);

		if(static::$use_un) {
			static::$v[static::SIG]=static::SIG."uv.ixml";
			$en[static::SIG ."prompt_username"]="Username";
			$de[static::SIG ."prompt_username"]="Benutzername";

			$en[static::SIG ."errors_username_exists"]="This username already exists on the system. Please choose another.";
			$de[static::SIG ."errors_username_exists"]="Dieser Benutzername wird bereits verwendet. Bitte wählen Sie einen anderen Benutzernamen.";

			$en[static::SIG ."errors_no_username"]="You need to enter a username.";
			$de[static::SIG ."errors_no_username"]="Bitte geben Sie Ihren Benutzernamen ein.";

			$en[static::SIG ."errors_new_pw_too_un_simple"]=" Passwords must be different from your username!";
			$de[static::SIG ."errors_new_pw_too_un_simple"]=" Passwort und Benutzername dürfen nicht übereinstimmen.";
		} else {
			static::$v[static::SIG]=static::SIG."mv.ixml";
			$en[static::SIG ."errors_new_pw_too_em_simple"]=" Passwords must be different from your email!";
			$de[static::SIG ."errors_new_pw_too_em_simple"]=" Passwort und E-Mail-Adresse dürfen nicht übereinstimmen.";
		}
		//set these after any contextual changes;
		static::$v = array_replace(static::$v,$custom_views);

		Dict::set($en,'en');
		Dict::set($de,'de');
	}

}