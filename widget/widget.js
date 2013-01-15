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
    var script   = document.createElement('script');
    script.src = url;
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
    head.appendChild(script);
  };
  // use TN copy of jQuery, Google version at: https://ajax.googleapis.com/ajax/libs/jquery/1.3.2/jquery.min.js
  getScript('https://www.transitionnetwork.org/sites/all/modules/jquery_update/replace/jquery.js', function() {
  // jQuery now loaded
  setUpModal();
  });
}
else {
  // jQuery already here, carry on
  setUpModal();
};


/**
 * 2. Enable modal popup on button
 */
function setUpModal() {
  // belt and braces
  if (typeof jQuery == 'undefined') {
    alert('PSE: No jQuery loaded');
    return;
  }
//jQuery(document)
  jQuery(document).ready(function() {
    // attach a click handler to the widget open button link
    jQuery('.tn-pse-widget-container .entry-widget-launcher').click(function () {
      // insert entry widget iFrame if not already present
      if (jQuery('.tn-pse-widget-container .entry-widget').length == 0) {
        jQuery('.tn-pse-widget-container .modal-inner').append('<iframe class="entry-widget" frameborder="0" style="display: hidden; width: 100%; height: 100%; border: none; overflow: hidden;"></iframe>');
        jQuery('.tn-pse-widget-container .entry-widget').attr('src', tn_pse_entry_widget_url);
        jQuery('.tn-pse-widget-container .entry-widget').fadeIn(1200);
      }

      // show modal overlay slowly-ish to allow it to load
      jQuery('.tn-pse-widget-container .modal-mask').fadeIn(500);
      return false;
    });
    // attach a click handler to the widget close link
    jQuery('.tn-pse-widget-container .modal-button-close').click(function () {
      jQuery('.tn-pse-widget-container .modal-mask').fadeOut();
    });
  });
}
