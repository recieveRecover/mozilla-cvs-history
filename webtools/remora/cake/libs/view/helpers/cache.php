<?php
/* SVN FILE: $Id: cache.php,v 1.1 2006/08/27 01:54:33 shaver%mozilla.org Exp $ */

/**
 * Short description for file.
 *
 * Long description for file
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
 * @since			CakePHP v 1.0.0.2277
 * @version			$Revision: 1.1 $
 * @modifiedby		$LastChangedBy: phpnut $
 * @lastmodified	$Date: 2006/08/27 01:54:33 $
 * @license			http://www.opensource.org/licenses/mit-license.php The MIT License
 */

/**
 * Short description for file.
 *
 * Long description for file
 *
 * @package		cake
 * @subpackage	cake.cake.libs.view.helpers
 */
class CacheHelper extends Helper{
/**
 * Array of strings replaced in cached views.
 * The strings are found between <cake:nocache><cake:nocache> in views
 *
 * @var array
 */
	 var $replace = array();
/**
 * Array of string that are replace with there var replace above.
 * The strings are any content inside <cake:nocache><cake:nocache> and includes the tags in views
 *
 * @var array
 */
	 var $match = array();
/**
 * holds the View object passed in final call to CacheHelper::cache()
 *
 * @var object
 */
	 var $view;

/**
 * Enter description here...
 *
 * @param string $file
 * @param string $out
 * @param string $cache
 * @return view ouput
 */
	 function cache($file, $out, $cache = false) {
		  if (is_array($this->cacheAction)) {
				$check  =str_replace('/', '_', $this->here);
				$replace=str_replace('/', '_', $this->base);
				$match  =str_replace($this->base, '', $this->here);
				$match  =str_replace('//', '/', $match);
				$match  =str_replace('/' . $this->controllerName . '/', '', $match);
				$check  =str_replace($replace, '', $check);
				$check  =str_replace('_' . $this->controllerName . '_', '', $check);
				$check  =convertSlash($check);
				$keys   =str_replace('/', '_', array_keys($this->cacheAction));
				$found  =array_keys($this->cacheAction);
				$index  =null;
				$count  =0;

				foreach($keys as $key => $value) {
					 if (strpos($check, $value) === 0) {
						  $index=$found[$count];
						  break;
					 }

					 $count++;
				}

				if (isset($index)) {
					 $pos1=strrpos($match, '/');
					 $char=strlen($match) - 1;

					 if ($pos1 == $char) {
						  $match = substr($match, 0, $char);
					 }

					 $key=$match;
				} elseif($this->action == 'index') {
					 $index = 'index';
				}

				if (isset($this->cacheAction[$index])) {
					 $cacheTime = $this->cacheAction[$index];
				} else {
					 $cacheTime = 0;
				}
		  } else {
				$cacheTime = $this->cacheAction;
		  }

		  if ($cacheTime != '' && $cacheTime > 0) {
				$this->__parseFile($file, $out);

				if ($cache === true) {
					 $cached=$this->__parseOutput($out);
					 $this->__writeFile($cached, $cacheTime);
				}

				return $out;
		  } else {
				return $out;
		  }
	 }

/**
 * Enter description here...
 *
 * @param string $file
 * @param boolean $cache
 */
	 function __parseFile($file, $cache) {
		  if (is_file($file)) {
				$file = file_get_contents($file);
		  } else if($file = fileExistsInPath($file)) {
				$file = file_get_contents($file);
		  }

		  preg_match_all('/(<cake:nocache>(?<=<cake:nocache>)[\\s\\S]*?(?=<\/cake:nocache>)<\/cake:nocache>)/i',
							  $cache,
							  $oresult,
							  PREG_PATTERN_ORDER);
		  preg_match_all('/(?<=<cake:nocache>)([\\s\\S]*?)(?=<\/cake:nocache>)/i', $file, $result,
							  PREG_PATTERN_ORDER);

		  if (!empty($result['0'])) {
				$count=0;

				foreach($result['0'] as $result) {
					 if (isset($oresult['0'][$count])) {
						  $this->replace[]=$result;
						  $this->match[]  =$oresult['0'][$count];
					 }

					 $count++;
				}
		  }
	 }

/**
 * Enter description here...
 *
 * @param sting $cache
 * @return string with all replacements made to <cake:nocache><cake:nocache>
 */
	 function __parseOutput($cache) {
		  $count=0;

		  if (!empty($this->match)) {
				foreach($this->match as $found) {
					 $cache = str_replace($found, $this->replace[$count], $cache);
					 $count++;
				}

				return $cache;
		  }

		  return $cache;
	 }

/**
 * Enter description here...
 *
 * @param string $file
 * @param sting $timestamp
 * @return cached view
 */
	 function __writeFile($file, $timestamp) {
		  $now=time();

		  if (is_numeric($timestamp)) {
				$cacheTime = $now + $timestamp;
		  } else {
				$cacheTime = $now + strtotime($timestamp);
		  }

		  $cache=convertSlash($this->here) . '.php';

		  $file='<!--cachetime:' . $cacheTime . '-->
					<?php
					loadController(\'' . $this->view->name . '\');
					$this->controller = new ' . $this->view->name . 'Controller();
					$this->helpers = unserialize(\'' . serialize($this->view->helpers) . '\');
					$this->base = \'' . $this->view->base . '\';
					$this->webroot = \'' . $this->view->webroot . '\';
					$this->here = \'' . $this->view->here . '\';
					$this->params = unserialize(\'' . serialize($this->view->params) . '\');
					$this->action = unserialize(\'' . serialize($this->view->action) . '\');
					$this->data = unserialize(\'' . serialize($this->view->data) . '\');
					$this->themeWeb = \'' . $this->view->themeWeb . '\';
					$this->plugin = \'' . $this->view->plugin . '\';
					$loadedHelpers = array();
					$loadedHelpers = $this->_loadHelpers($loadedHelpers, $this->helpers);
					foreach(array_keys($loadedHelpers) as $helper)
					{
						$replace = strtolower(substr($helper, 0, 1));
						$camelBackedHelper = preg_replace(\'/\\w/\', $replace, $helper, 1);
						${$camelBackedHelper} =& $loadedHelpers[$helper];

						if(isset(${$camelBackedHelper}->helpers) && is_array(${$camelBackedHelper}->helpers))
						{
							foreach(${$camelBackedHelper}->helpers as $subHelper)
							{
								${$camelBackedHelper}->{$subHelper} =& $loadedHelpers[$subHelper];
							}
						}
						$this->loaded[$camelBackedHelper] = (${$camelBackedHelper});
					}
					?>
					' . $file;
		  return cache('views' . DS . $cache, $file, $timestamp);
	 }
}
?>