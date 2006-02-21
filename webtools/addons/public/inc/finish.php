<?php
/**
 * Finish script.
 *
 * @package amo
 * @subpackage inc
 */



/**
 * If we do not have a pageType, we need to set one.
 */
if (empty($pageType)) {
  $pageType = "default";
}



/**
 * Compiling and fetching templates.
 *
 * Now that we're here, we need to throw together our output using our templates.
 * pageOutput will hold any data streamed to the user.
 */
$pageOutput = '';



/**
 * Fetch and store page header.
 *
 * $currentTab is set in init.php or the calling script,
 * and corresponds to the selected tab in the header.tpl template.
 *
 * $compileId is set in init.php and corresponds to the current app.
 *
 * Our special case is if the pageType is xml, where we would not display the HTML headers.
 */
if ($pageType != 'xml') {
    $pageOutput .= $tpl->fetch('inc/wrappers/' . $pageType . '-header.tpl', $GLOBALS['currentTab'], $GLOBALS['compileId']);
}



/**
 * Fetch and store our page content.
 *
 * $cacheId is set in init.php or the colling script.  It
 * is unique for any number of combinations of parameters (GET, typically).
 *
 * $compileId is set in init.php and corresponds to the current app.
 */
$pageOutput .= $tpl->fetch($content,$cacheId,$compileId);



/**
 * Fetch and store our page footer.  We do not pass it a cacheId or compileId
 * because the footer doesn't change based on parameters.
 *
 * Our special case is if the pageType is xml, where we would not display the HTML headers.
 */
if ($pageType != 'xml') {
    $pageOutput .= $tpl->fetch('inc/wrappers/' . $pageType . '-footer.tpl');
}



/**
 * If our config says so, the page should be cached.
 */
if (!empty($cache_config[SCRIPT_NAME])) {

    // Save our page output to our cache.
    $cache->save($pageOutput,$cacheLiteId,SCRIPT_NAME);
}



/**
 * If we have specified a custom content-type for this script, echo the header here.
 */
if (!empty($contentType_config[SCRIPT_NAME])) {
    header('Content-type: '.$contentType_config[SCRIPT_NAME]);
}



/**
 * If we get here, there is no caching.
 * We should just dump the output.
 */
echo $pageOutput;
?>
