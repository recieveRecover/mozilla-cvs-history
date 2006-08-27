<?php
/* SVN FILE: $Id: paths.php,v 1.1 2006/08/27 01:54:30 shaver%mozilla.org Exp $ */
/**
 * Short description for file.
 *
 * In this file you set paths to different directories used by Cake.
 *
 * PHP versions 4 and 5
 *
 * CakePHP :  Rapid Development Framework <http://www.cakephp.org/>
 * Copyright (c)	2006, Cake Software Foundation, Inc.
 *								1785 E. Sahara Avenue, Suite 490-204
 *								Las Vegas, Nevada 89104
 *
 * Licensed under The MIT License
 * Redistributions of files must retain the above copyright notice.
 *
 * @filesource
 * @copyright		Copyright (c) 2006, Cake Software Foundation, Inc.
 * @link				http://www.cakefoundation.org/projects/info/cakephp CakePHP Project
 * @package			cake
 * @subpackage		cake.cake.app.config
 * @since			CakePHP v 0.2.9
 * @version			$Revision: 1.1 $
 * @modifiedby		$LastChangedBy: phpnut $
 * @lastmodified	$Date: 2006/08/27 01:54:30 $
 * @license			http://www.opensource.org/licenses/mit-license.php The MIT License
 */
/**
 * If the index.php file is used instead of an .htaccess file
 * or if the user can not set the web root to use the public
 * directory we will define ROOT there, otherwise we set it
 * here.
 */
	if(!defined('ROOT')) {
		define ('ROOT', '../');
	}
	if(!defined('WEBROOT_DIR')) {
		define ('WEBROOT_DIR', 'webroot');
	}
/**
 * Path to the application's directory.
 */
	define ('CAKE', CORE_PATH.'cake'.DS);
/**
 * Path to the application's directory.
 */
	define ('APP', ROOT.DS.APP_DIR.DS);
/**
 * Path to the application's models directory.
 */
	define ('MODELS', APP.'models'.DS);
/**
 * Path to the application's controllers directory.
 */
	define ('CONTROLLERS', APP.'controllers'.DS);
/**
 * Path to the application's controllers directory.
 */
	define ('COMPONENTS', CONTROLLERS.'components'.DS);
/**
 * Path to the application's views directory.
 */
	define ('VIEWS', APP.'views'.DS);
/**
 * Path to the application's helpers directory.
 */
	define ('HELPERS', VIEWS.'helpers'.DS);
/**
 * Path to the application's view's layouts directory.
 */
	define ('LAYOUTS', VIEWS.'layouts'.DS);
/**
 * Path to the application's view's elements directory.
 * It's supposed to hold pieces of PHP/HTML that are used on multiple pages
 * and are not linked to a particular layout (like polls, footers and so on).
 */
	define ('ELEMENTS', VIEWS.'elements'.DS);
/**
 * Path to the configuration files directory.
 */
	define ('CONFIGS', APP.'config'.DS);
/**
 * Path to the libs directory.
 */
	define ('INFLECTIONS', CAKE.'config'.DS.'inflections'.DS);
/**
 * Path to the libs directory.
 */
	define ('LIBS', CAKE.'libs'.DS);
/**
 * Path to the modules directory.
 */
	define ('MODULES', ROOT.DS.'modules'.DS);
/**
 * Path to the public directory.
 */
	define ('CSS', WWW_ROOT.'css'.DS);
/**
 * Path to the public directory.
 */
	define ('JS', WWW_ROOT.'js'.DS);
/**
 * Path to the scripts direcotry.
 */
	define('SCRIPTS', CAKE.'scripts'.DS);
/**
 * Path to the tests directory.
 */
	define ('TESTS', APP.'tests'.DS);
/**
 * Path to the controller test directory.
 */
	define ('CONTROLLER_TESTS', TESTS.APP_DIR.DS.'cases'.DS.'controllers'.DS);
/**
 * Path to the helpers test directory.
 */
	define ('HELPER_TESTS', TESTS.APP_DIR.DS.'cases'.DS.'views'.DS.'helpers'.DS);
/**
 * Path to the models' test directory.
 */
	define ('MODEL_TESTS', TESTS.APP_DIR.DS.'cases'.DS.'models'.DS);
/**
 * Path to the lib test directory.
 */
	define ('LIB_TESTS', TESTS.'lib'.DS);
/**
 * Path to the temporary files directory.
 */
	define ('TMP', APP.'tmp'.DS);
/**
 * Path to the logs directory.
 */
	define ('LOGS', TMP.'logs'.DS);
/**
 * Path to the cache files directory. It can be shared between hosts in a multi-server setup.
 */
	define('CACHE', TMP.'cache'.DS);
/**
 * Path to the vendors directory.
 */
	define ('VENDORS', CAKE_CORE_INCLUDE_PATH.DS.'vendors'.DS);
/**
 * Path to the Pear directory
 * The purporse is to make it easy porting Pear libs into Cake
 * without setting the include_path PHP variable.
 */
	define ('PEAR', VENDORS.'Pear'.DS);
/**
 *  Full url prefix
 */
	$s = null;
	$https = env('HTTPS');
	if ( (isset($https) && $https =='on' )) {
		$s ='s';
	}
	unset($https);
	$httpHost = env('HTTP_HOST');

	if (isset($httpHost)) {
		define('FULL_BASE_URL', 'http'.$s.'://'.$httpHost);
	}
	unset($httpHost);
/**
 * Web path to the public images directory.
 */
	define ('IMAGES_URL', 'img/');
/**
 * Web path to the CSS files directory.
 */
	define ('CSS_URL', 'css/');
/**
 * Web path to the js files directory.
 */
	define ('JS_URL', 'js/');
?>