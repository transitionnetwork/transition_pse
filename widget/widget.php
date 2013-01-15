<?php
$accout_uid = 1;
$initiative_id = 191;
$view_sidebar_url = "/pse/view/sidebar/$accout_uid/$initiative_id";
$view_full_url = "/pse/view/full/$accout_uid/$initiative_id";
$entry_url = "/pse/entry/add-project/$accout_uid/$initiative_id";
$entry_fallback_url = "/pse/entry/add-project/$accout_uid/$initiative_id";
$asset_url = "/sites/all/modules/custom/transition_pse/widget";

?><!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
  "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
  <title>PSE widget structure</title>
  <meta http-equiv="content-type" content="text/html;charset=utf-8" />
  <meta name="generator" content="Geany 0.20" />
</head>

<body>
  <div id="pretend-body" style="width: 80%; float: left">
    <h1>Widget structure</h1>
    <p>This is the sidebar widget layout. Please change styles and anything within <code>a.entry-widget-launcher</code>,
      but all class names should stay the same, while the structure (divs etc) should be altered only after discussion.</p>
    <p>All <code>style</code> attributes can be edited -- QUESTION: we're probably best not to import a CSS file since it's likely to
      clash with the host page's CSS and is another file to include... All the CSS magic will be in the iFrame</p>
    <p>The 'pretent sidebar' div (<code>div#pretend-sidebar</code>) is just to hold the widget container as if in a page and is not part of the widget.</p>
    <p>View the source to see the code. This body text is held in a 'pretend-body' div.</p>
    <h3>CSS classes &amp; structure</h3>
    <p>Will be used by jQuery etc to manipulate the code...</p>
    <ul>
      <li><strong>div.tn-pse-widget-container</strong> - container wrapper for the whole thing. To avoid
        clashes, this must be used to specifically reference all items within it when using CSS and JS.</li>
      <li><strong>div.widget-upper</strong> - upper wrapper inside container.</li>
      <li><strong>iframe.view-widget</strong> - iFrame within upper section</li>
      <li><strong>div.widget-lower</strong> - lower wrapper inside container.</li>
      <li><strong>a.entry-widget-launcher</strong> - <code>&lt;a&gt;</code> tag with image for add button</li>
      <li><strong>div.modal-mask</strong> - a shadow mask or 'cover' for the page when modal dialog is over it</li>
      <li><strong>div.modal-outer</strong> - outer of modal dialog, containing close button and inner</li>
      <li><strong>a.modal-button-close</strong> - outer of modal dialog, containing close button and inner</li>
      <li><strong>div.modal-inner</strong> - inner of modal dialog, contains iframe for entry widget</li>
      <li><strong>iframe.entry-widget</strong> - an iFrame to the entry widget. Added by jQuery not initially present</li>
    </ul>
  </div>

  <div id="pretend-sidebar" style="width: 19.9%; float: left; padding-top: 30px">
    <!-- TN-PSE: widget begin -->
    <script type="text/javascript">var tn_pse_entry_widget_url = '<?php print $entry_url; ?>';</script>
    <script type="text/javascript" src="<?php print $asset_url; ?>/tn-pse-widget.js"></script>
    <link rel="stylesheet" type="text/css" href="<?php print $asset_url; ?>/tn-pse-widget.css" />
    <div class="tn-pse-widget-container">
      <div class="widget-upper">
        <iframe class="view-widget" src="<?php print $view_sidebar_url; ?>" frameborder="0"
          style="width: 100%; height: 300px; border: none; overflow: hidden;"></iframe>
      </div>
      <div class="widget-lower">
        <div class="grey-box"><p><strong>Are you involved or know of a community project that could be shared?</strong></p>
			<a href="<?php print $entry_fallback_url; ?>" class="entry-widget-launcher"><span>Add your project!</span></a>
		</div>
      </div>
      <div class="modal-mask" style="display: none">
        <div class="modal-outer">
          <a href="#" class="modal-button-close">Close</a>
          <div class="modal-inner"><!-- iFrame with class 'entry-widget' will be inserted here --></div>
        </div>
      </div>
    </div>
    <!-- TN-PSE: widget end -->
  </div>
</body>
</html>
