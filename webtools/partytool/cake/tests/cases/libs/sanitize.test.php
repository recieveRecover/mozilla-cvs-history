<?php
/* SVN FILE: $Id: sanitize.test.php,v 1.1 2007/11/19 09:16:15 rflint%ryanflint.com Exp $ */
/**
 * Short description for file.
 *
 * Long description for file
 *
 * PHP versions 4 and 5
 *
 * CakePHP(tm) Tests <https://trac.cakephp.org/wiki/Developement/TestSuite>
 * Copyright 2005-2007, Cake Software Foundation, Inc.
 *								1785 E. Sahara Avenue, Suite 490-204
 *								Las Vegas, Nevada 89104
 *
 *  Licensed under The Open Group Test Suite License
 *  Redistributions of files must retain the above copyright notice.
 *
 * @filesource
 * @copyright		Copyright 2005-2007, Cake Software Foundation, Inc.
 * @link				https://trac.cakephp.org/wiki/Developement/TestSuite CakePHP(tm) Tests
 * @package			cake.tests
 * @subpackage		cake.tests.cases.libs
 * @since			CakePHP(tm) v 1.2.0.5428
 * @version			$Revision: 1.1 $
 * @modifiedby		$LastChangedBy: phpnut $
 * @lastmodified	$Date: 2007/11/19 09:16:15 $
 * @license			http://www.opensource.org/licenses/opengroup.php The Open Group Test Suite License
 */
uses('sanitize');
/**
 * Short description for class.
 *
 * @package    cake.tests
 * @subpackage cake.tests.cases.libs
 */
class SanitizeTest extends UnitTestCase {

	function testEscapeAlphaNumeric() {
		$resultAlpha = Sanitize::escape('abc', 'default');
		$this->assertEqual($resultAlpha, 'abc');

		$resultNumeric = Sanitize::escape('123', 'default');
		$this->assertEqual($resultNumeric, '123');

		$resultNumeric = Sanitize::escape(1234, 'default');
		$this->assertEqual($resultNumeric, 1234);

		$resultNumeric = Sanitize::escape(1234.23, 'default');
		$this->assertEqual($resultNumeric, 1234.23);

		$resultNumeric = Sanitize::escape('#1234.23', 'default');
		$this->assertEqual($resultNumeric, '#1234.23');
	}

	function testClean() {
		$string = 'test & "quote" \'other\' ;.$ symbol.' . "\r" . 'another line';
		$expected = 'test &amp; &quot;quote&quot; &#39;other&#39; ;.$ symbol.another line';
		$result = Sanitize::clean($string);
		$this->assertEqual($result, $expected);

		$string = 'test & "quote" \'other\' ;.$ symbol.' . "\r" . 'another line';
		$expected = 'test & ' . Sanitize::escape('"quote"') . ' ' . Sanitize::escape('\'other\'') . ' ;.$ symbol.another line';
		$result = Sanitize::clean($string, array('encode' => false));
		$this->assertEqual($result, $expected);

		$string = 'test & "quote" \'other\' ;.$ \\$ symbol.' . "\r" . 'another line';
		$expected = 'test & "quote" \'other\' ;.$ $ symbol.another line';
		$result = Sanitize::clean($string, array('encode' => false, 'escape' => false));
		$this->assertEqual($result, $expected);

		$string = 'test & "quote" \'other\' ;.$ \\$ symbol.' . "\r" . 'another line';
		$expected = 'test & "quote" \'other\' ;.$ \\$ symbol.another line';
		$result = Sanitize::clean($string, array('encode' => false, 'escape' => false, 'dollar' => false));
		$this->assertEqual($result, $expected);

		$string = 'test & "quote" \'other\' ;.$ symbol.' . "\r" . 'another line';
		$expected = 'test & "quote" \'other\' ;.$ symbol.' . "\r" . 'another line';
		$result = Sanitize::clean($string, array('encode' => false, 'escape' => false, 'carriage' => false));
		$this->assertEqual($result, $expected);
		
		$array = array(array('test & "quote" \'other\' ;.$ symbol.' . "\r" . 'another line'));
		$expected = array(array('test &amp; &quot;quote&quot; &#39;other&#39; ;.$ symbol.another line'));
		$result = Sanitize::clean($array);
		$this->assertEqual($result, $expected);

		$array = array(array('test & "quote" \'other\' ;.$ \\$ symbol.' . "\r" . 'another line'));
		$expected = array(array('test & "quote" \'other\' ;.$ $ symbol.another line'));
		$result = Sanitize::clean($array, array('encode' => false, 'escape' => false));
		$this->assertEqual($result, $expected);
		
		
		
		
		
		
		
	}
}
?>
