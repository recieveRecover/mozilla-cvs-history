<?php
/* SVN FILE: $Id: model.php,v 1.1 2006/08/27 01:54:32 shaver%mozilla.org Exp $ */

/**
 * Object-relational mapper.
 *
 * DBO-backed object data model, for mapping database tables to Cake objects.
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
 * @subpackage		cake.cake.libs.model
 * @since			CakePHP v 0.10.0.0
 * @version			$Revision: 1.1 $
 * @modifiedby		$LastChangedBy: phpnut $
 * @lastmodified	$Date: 2006/08/27 01:54:32 $
 * @license			http://www.opensource.org/licenses/mit-license.php The MIT License
 */

/**
 * Load the model class based on the version of PHP.
 *
 */
if (phpversion() < 5) {
	 require(LIBS . 'model' . DS . 'model_php4.php');

	 if (function_exists("overload")) {
		  overload("Model");
	 }
} else {
	 require(LIBS . 'model' . DS . 'model_php5.php');
}
?>