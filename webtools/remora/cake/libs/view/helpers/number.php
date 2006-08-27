<?php
/* SVN FILE: $Id: number.php,v 1.1 2006/08/27 01:54:33 shaver%mozilla.org Exp $ */

/**
 * Number Helper.
 *
 * Methods to make numbers more readable.
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
 * @subpackage		cake.cake.libs.view.helpers
 * @since			CakePHP v 0.10.0.1076
 * @version			$Revision: 1.1 $
 * @modifiedby		$LastChangedBy: phpnut $
 * @lastmodified	$Date: 2006/08/27 01:54:33 $
 * @license			http://www.opensource.org/licenses/mit-license.php The MIT License
 */

/**
 * Number helper library.
 *
 * Methods to make numbers more readable.
 *
 * @package 	cake
 * @subpackage	cake.cake.libs.view.helpers
 */
class NumberHelper extends Helper{
/**
 * Formats a number with a level of precision.
 *
 * @param  float	$number	A floating point number.
 * @param  integer $precision The precision of the returned number.
 * @return float Enter description here...
 * @static
 */
	 function precision($number, $precision = 3) {
		  return sprintf("%01.{$precision}f", $number);
	 }

/**
 * Returns a formatted-for-humans file size.
 *
 * @param integer $length Size in bytes
 * @return string Human readable size
 * @static
 */
	 function toReadableSize($size) {
		  switch($size)
				{
				case 1: return '1 Byte';

				case $size < 1024: return $size . ' Bytes';

				case $size < 1024 * 1024: return NumberHelper::precision($size / 1024, 0) . ' KB';

				case $size < 1024 * 1024 * 1024: return NumberHelper::precision($size / 1024 / 1024, 2) . ' MB';

				case $size < 1024 * 1024 * 1024 * 1024:
					return NumberHelper::precision($size / 1024 / 1024 / 1024, 2) . ' GB';

				case $size < 1024 * 1024 * 1024 * 1024 * 1024:
					return NumberHelper::precision($size / 1024 / 1024 / 1024 / 1024, 2) . ' TB';
				}
	 }

/**
 * Formats a number into a percentage string.
 *
 * @param float $number A floating point number
 * @param integer $precision The precision of the returned number
 * @return string Percentage string
 * @static
 */
	 function toPercentage($number, $precision = 2) {
		  return NumberHelper::precision($number, $precision) . '%';
	 }
}
?>
