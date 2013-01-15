Drupal.behaviors.unifiedLogin = function (context) {                                                                                                                            
  // For compatibility with non-js browsers, we hide the #register-form here
  // it is important that this CSS gets added early so the form doesn't
  // appear while the page is loading (before $(document).ready() fires)
  $('<style type="text/css">.pse-unified #register-form, .pse-unified #login-form {display:none;}</style>').appendTo('head');
  $(document).ready(function() {
    // Attach behaviors to the links so that they show/hide forms appropriately.
    $('.pse-unified #register-link').click(function() {
      $(this).addClass('lt-active').blur();
      $('.pse-unified #login-link').removeClass('lt-active');
      $('.pse-unified #register-form').show();
      $('.pse-unified #login-form').hide();
      return false;
    });
    $('.pse-unified #login-link').click(function() {
      $(this).addClass('lt-active').blur();
      $('.pse-unified #register-link').removeClass('lt-active');
      $('.pse-unified #login-form').show();
      $('.pse-unified #register-form').hide();
      return false;
    });

    switch(Drupal.settings.lt.activeForm) {
      case 'register':
        $('.pse-unified #register-link').click();
        break;
      case 'login':
      default:
        $('.pse-unified #login-link').click();
        break;
    }
  });
}
