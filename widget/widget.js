/**
 * PSE jQuery modal popup
 * ======================
 * Transition Network - Project Sharing Engine
 * by Jim Kirkpatrick, March 2012 -- bad d0t octopus at gmail dott com
 *
 * This code imports jQuery if absent from containing page, and sets up modal
 * dialog on click of 'add project' button.
 *
 * Note: all functions should start with tn_pse_ namespace.
 */


/**
 * 1. Load jQuery if only if not already in the page
 *    from: http://noodle.mx/scripts/widgets.js
 */
// Only do anything if jQuery isn't defined
if (typeof jQuery == 'undefined') {
  function getScript(url, success) {
    var script = document.createElement('script');
    script.src = url;
    // script.async = true;
    var head = document.getElementsByTagName('head')[0],
    done = false;

    // Attach handlers for all browsers
    script.onload = script.onreadystatechange = function() {
      if (!done && (!this.readyState || this.readyState == 'loaded' || this.readyState == 'complete')) {
      done = true;
        // callback function provided as param
        success();
        script.onload = script.onreadystatechange = null;
        head.removeChild(script);
      };
    };
    // add to head
    head.appendChild(script);
  };
  // use TN copy of jQuery, Google version at: https://ajax.googleapis.com/ajax/libs/jquery/1.3.2/jquery.min.js
  getScript('https://www.transitionnetwork.org/sites/all/modules/contrib/jquery_update/replace/jquery.js', function() {
  // jQuery now loaded
  tn_pse_widget_setUpModal();
  });
}
else {
  // jQuery already here, carry on
  tn_pse_widget_setUpModal();
};


/**
 * 2. Enable modal popup on button
 */
function tn_pse_widget_setUpModal() {
  // belt and braces
  if (typeof jQuery == 'undefined') {
    alert('PSE: No jQuery loaded');
    return;
  }
//jQuery(document)
  jQuery(document).ready(function() {

    // attach a click handler to the widget entry 'add' button link
    jQuery('.tn-pse-widget-container .tn-pse-widget-entry-widget-launcher').click(function () {
      // display the entry url
      tn_pse_widget_showModal(tn_pse_widget_entry_url);
      return false;
    });

    // attach a click handler to the widget about button link
    jQuery('.tn-pse-widget-container .tn-pse-widget-about-widget-launcher').click(function () {
      // display the entry url
      tn_pse_widget_showModal(tn_pse_widget_about_url);
      return false;
    });

    // attach a click handler to the widget close link
    jQuery('.tn-pse-widget-container .tn-pse-widget-modal-button-close').click(function () {
      jQuery('.tn-pse-widget-container .tn-pse-widget-modal-mask').fadeOut();
    });
  });
}

function tn_pse_widget_showModal(target_url) {
  // insert entry widget iFrame if not already present
  if (jQuery('.tn-pse-widget-container .tn-pse-widget-modal-frame').length == 0) {
    jQuery('.tn-pse-widget-container .tn-pse-widget-modal-inner').append('<iframe class="tn-pse-widget-modal-frame" frameborder="0" style="display: hidden; width: 100%; height: 100%; border: none; overflow: hidden;"></iframe>');
  }
  // set url
  jQuery('.tn-pse-widget-container .tn-pse-widget-modal-frame').attr('src', target_url);
  // show modal overlay slowly-ish to allow it to load
  jQuery('.tn-pse-widget-container .tn-pse-widget-modal-frame').fadeIn(1200);
  jQuery('.tn-pse-widget-container .tn-pse-widget-modal-mask').fadeIn(500);
}