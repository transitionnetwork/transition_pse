<?php
// $Id: pse-page.tpl.php

/**
 * @file pse-page.tpl.php
 * Wraps PSE content in head and foot elements.
 *
 * Available variables:
 * - $header: Header HTML from pse-page-header.tpl.php
 * - $content: Main content HTML
 * - $header: Footer HTML from pse-page-footer.tpl.php
 */
?>
<div id="pse-page">
  <header id="pse-header">
    <div class="inner"><?php print $header; ?></div>
  </header>
  <div class="clear-block">
  <div id="pse-content">
    <div class="inner"><?php print $content; ?></div>
  </div>
  </div>
  <footer id="pse-footer">
    <div class="inner"><?php print $footer; ?></div>
  </footer>
</div>
