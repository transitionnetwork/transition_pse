<?php
/**
 * @file
 *
 * Include file for PSE widget generation and related tasks.
 */

/**
 * Get Widget page callback
 *
 * Only users who are the web point of contact are able to see this page.
 * The page will show them a 'sign up' view if they're not with 'widget owner',
 * or the page to get the code if they are.
 */
function transition_pse_widget_get($account_uid = -1, $type = 'sidebar', $initiative_id = -1) {
  // belt & braces ensure user and type are ok
  global $user;
  if ($account_uid == -1) $account_uid = $user->uid;
  if ($type <> 'full' && $type <> 'button') $type = 'sidebar';

  // load this user, prepare initiative data, load nids
  $account = user_load($account_uid);
  $initiatives = array(
    'selected' => $initiative_id,
    'options' => transition_pse_web_point_of_contact_initiatives($account),
  );

  // add in choice from url path, if present
  if ($initiative_id <> -1 && !in_array($initiative_id, $initiatives['options'])) {
    $initiatives['options'][$initiative_id] = $initiative_id;
  }

  // load inis titles from nids, find match passed in if poss.
  $initiative_found = FALSE;
  foreach ($initiatives['options'] as $nid) {
    if ($node = node_load(array('nid' => $nid, 'status' => 1))) {
      // ini loaded ok, add title to options
      $initiatives['options'][$nid] = $node->title;
      // note we've found the $initiative_id
      if ($nid == $initiative_id) $initiatives['ok'] = TRUE;
    }
    else {
      // can't load ini (unpublished?), remove option
      unset($initiatives['options'][$nid]);
    }
  }

  // warn & bail if no inis to chose from
  if (!isset($initiatives['ok']) || !transition_pse_web_point_of_contact_initiatives_access()) {
    $output .= t('<h2>Sorry, you must be the <em>Web Point of Contact</em> for your initiative to continue</h2>');
    $output .= t('<p><strong>Unfortunately, you cannot use the widget until you have been set up as the "Web Point of Contact" for your local initiative.</strong></p>');
    $output .= t(
      '<p>Please find your initiative in the <a href="!link">Initiatives Directory</a> and either edit it (if you have that ability), or contact the person who is marked as the "Primary point of contact".</p>',
      array('!link' => '/initiatives')
    );
    $output .= t('<p>Remember to come back here when you\'re done, and please <a href="!link">contact us</a> if you have any questions or problems.</hp>', array('!link' => '/contact'));
    return $output;
  }

  // if we're already a
  if (in_array('widget owner', array_values($account->roles))) {
    return transition_pse_widget_generate($account, $type, $initiatives);
  }
  else {
    return drupal_get_form('transition_pse_widget_owner_signup_form', $account);
  }
}

/**
 * Presents a signup form for the user to agree to T&Cs and get newsletter before
 * they can get the 'widget owner' role, and the widget code.
 */
function transition_pse_widget_owner_signup_form($form_state, $account) {
  global $user;
  $form = array();
  $form_newsletter = array();
  // get mailchimp code from other include ONLY IF we're not being admin
  if ($user->uid == $account->uid) {
    module_load_include('inc', 'transition_pse', 'transition_pse.pages');
    // get user's subscription status (see mailchimp.module, line ~222)
    $q = _mailchimp_get_api_object();
    $memberinfo = $q->listMemberInfo(TRANSITION_PSE_MC_LID, $account->mail);
    // check status, add form if user not already subscribed
    if ($memberinfo['status'] == 'unsubscribed') {
      $form_newsletter = transition_pse_newsletter_form($form_state);
    }
  }
  // decide on message
  if (isset($form_newsletter['wrapper' . TRANSITION_PSE_MC_LID])) {
    // user not signed up to mailchimp newsletter -- need to add signup checkbox
    $markup = t('<h3>Please agree to the terms and subscribe to the Widget Updates newsletter to continue</h3>');
  }
  else {
    // user already signed up
    $markup = t('<h3>Please agree to the terms to continue</h3>');
  }
  // build form
  $form['account'] = array(
    '#type' => 'hidden',
    '#value' => $account->uid,
  );
  $form['note'] = array(
    '#type' => 'markup',
    '#value' => $markup,
  );
  $form['terms'] =  array(
    '#type' => 'checkbox',
    '#required' => TRUE,
    '#default_value' => TRUE,
    '#title' => t(
      'I want to be a widget owner and agree with the <a href="!link">Transition Network website terms and conditions</a>',
      array('!link' => '/terms')
    ),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('I agree, give me the widget!'),
    '#weight' => 50,
  );
  $form = array_merge($form, $form_newsletter);
  return $form;
}

/**
 * Submit handler for widget signup form
 */
function transition_pse_widget_owner_signup_form_submit($form, &$form_state) {
// check we agreed to the terms
  if ($form_state['values']['terms']) {
    // Agreed, find and give user the 'widget owner' role.
    $roles = user_roles();
    $owner_rid = array_search('widget owner', $roles);
    // quick and dirty user role save
    db_query(
      "INSERT INTO {users_roles} (rid, uid) VALUES ('%d','%d')",
      $owner_rid,
      $form_state['values']['account']
    );
  }
  else {
    form_set_error(
      'terms',
      t(
        'You must agree to the <a href="!link">Transition Network website terms and conditions</a> to be able to get the widget.',
        array('!link' => '/terms')
      )
    );
  }
  // handle newsletter signup with existing code (only when user is acting for themselves)
  if ($form_state['values']['mailchimp_list_' . TRANSITION_PSE_MC_LID]) {
    module_load_include('inc', 'transition_pse', 'transition_pse.pages');
    transition_pse_newsletter_form_submit($form, &$form_state);
  }
}

/**
 * get widget code page callback
 * @todo break into smaller functions and use theme() more
 */
function transition_pse_widget_generate($account, $type, $initiatives) {
  $output = '';
  // set up options array for easy tweaking -- one day make overridable through UI
  $options = array(
    'add button text' => t('Add your project!'),
    'height' => 350 + ($type == 'full') * 250,
  );

  // add form to allow choice of sidebar/full and ini to relate to via UI/tabs/links etc.
  $path = base_path() . drupal_get_path('module', 'transition_pse');
  $output .= '<div class="help">' . t(
    '<strong>What\'s the Project Sharing Widget?</strong> You might like to learn more about the widget and its use with our <a href="@link_faq">FAQs</a> and <a href="@link_setup">setup guide</a>.',
    array('@link_faq' => '/pse/widget/faq', '@link_setup' => '/node/27590')
  ) . '</div>';
  $output .= t('<h3>Step 1: Choose your style and initiative</h3>');
  $output .= t(
    '<p><img src="@path" alt="Screenshots of full sidebar widgets on initiative websites" class="float-right" height="400" width="440"> Remember to click "Update widget" if you make a change to these settings.<p>',
    array('@path' => $path . '/images/PSE_widget-screenshots.jpg')
  );
  $output .= drupal_get_form('transition_pse_widget_generate_options_form', $type, $initiatives, $options, $account);


  if ($initiatives['ok']) {
    // make pse urls, ensure secure
    global $base_url;
    $base_url_secure = str_replace('http:', 'https:', $base_url);
    $base_url_insecure = str_replace('https:', 'http:', $base_url);
    $widget_urls = array(
      'view' => $base_url_insecure . "/pse/view/$type/" . $account->uid . "/" . $initiatives['selected'],
      'about' => $base_url_insecure . "/pse/about",
      'entry' => $base_url_secure . "/pse/entry/add-project/" . $account->uid . "/" . $initiatives['selected'],
      'entry nojs' => $base_url_secure . "/pse/entry/add-project/" . $account->uid . "/" . $initiatives['selected'],
    );
    // set up widget asset paths
    $assets = array(
      'js' => $base_url_secure . $path . '/widget/widget.js',
      'css' => $base_url_secure . $path . '/widget/widget.css',
    );

    // build widget HTML, put into page
    $widget_html = theme('pse_widget_container', $type, $widget_urls, $assets, $options);
    $output .= t('<h2>Step 2: Try out a demonstration</h2>');
    $output .= t('<p><strong>' . ucfirst($type) . ' widget for <em>' . $initiatives['options'][$initiatives['selected']] . '</em>:</strong></p>');
    if ($type == 'full') {
      $output .= t('<div class="help"><strong>Note</strong>: This "Full page" widget needs a new page on your website. Add this code to a new page you have created on your website.</div>');
    }
    $output .= '<div id="tn-pse-demo-container"' . ($type <> 'full' ? ' style="width: 235px; margin: 0 auto"' : '') . '>';
    $output .= $widget_html . '</div>';

    // spit out embed selection using Web Widgets module theme functions
    $output .= t('<h3>Step 3: Add this code to your site</h3>');
    $output .= theme('web_widgets_embed_code', $widget_html, 'iframe');

    // Show option to sign up to widget newsletter ONLY if user hasn't already done so.
    // @note This all smells a bit because the MC module doesn't have the best of APIs.
    $step = 4;
    if (module_exists('mailchimp')) {
      // get user's subscription status (see mailchimp.module, line ~222)
      $q = _mailchimp_get_api_object();
      $memberinfo = $q->listMemberInfo(TRANSITION_PSE_MC_LID, $account->mail);
      // check status, add form if user not already subscribed
      if ($memberinfo['status'] == 'unsubscribed') {
        $output .= t('<h3>Step !step: Keep updated about your widget</h3>', array('!step' => $step));
        module_load_include('inc', 'transition_pse', 'transition_pse.pages');
        $output .= drupal_get_form('transition_pse_newsletter_form', TRUE);
        // add another step
        $step++;
      }
    }

    //@todo theme functions!
    $output .= t('<h3>Step !step: Now the widget is on your website</h3>', array('!step' => $step));

    $output .= t('<p>To get projects added you now need to promote the widget: Put it in your
      newsletters, email-outs, link to it from your news items, and tell everyone about it.</p>');
    $output .= t(
      '<p>Once people start adding their proejcts, you will get the project submissions alerts
      via email. Please attend to these as soon as you can; it won\'t take long. You will be able to moderate
      the submissions in the <a href="@link"><em>Projects moderation</em> tab </a>of your user account.</p>',
      array('@link' => 'user/' . $account->uid . '/widget-moderation')
    );
  }
  else {
    // no initiative set, show message
    drupal_set_message('Choose an initiative and click <em>update widget</em> to get going.');

  }
  return $output;
}

/**
 * Form function for the widget generator. Allows user to chose type (sidebar vs
 * full) and initaitve if they have more that one associated with their account.
 */
function transition_pse_widget_generate_options_form($form_state, $type, $initiatives, $options, $account) {
  $form = array();
  $form['account'] = array(
    '#type' => 'hidden',
    '#value' => $account->uid,
  );
  $form['type'] = array(
      '#type' => 'radios',
      '#title' => t('Widget size & display type'),
      '#description' => t('Sidebar widgets are simple and are best suited to being placed in a sidebar, whereas the full widget has more functionality but needs more space.'),
      '#default_value' => $type,
      '#options' => array(
        'sidebar' => t('Sidebar widget'),
        'full' => t('Full page widget'),
        'button' => t('Button-only widget'),
      ),
    );
  $form['initiative'] = array(
    '#type' => 'radios',
    '#title' => t('Associated Initiative'),
    '#description' => t('Choose the Transition Initiative you are a Web Point of Contact for -- allows projects near your initiative to be shown.'),
    '#default_value' => $initiatives['selected'],
    '#options' => $initiatives['options'],
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update widget'));
  return $form;
}

/**
 * submit handler for transition_pse_widget_generate_options_form
 */
function transition_pse_widget_generate_options_form_submit($form, &$form_state) {
  $path = 'pse/widget/get/' . $form_state['values']['account'] . '/' . $form_state['values']['type'] . '/' . $form_state['values']['initiative'];
  drupal_goto($path);
}

/**
 * Implementation of hook_theme_registry_alter().
 */
function transition_pse_widget_theme_registry_alter(&$theme_registry) {
  if (!empty($theme_registry['web_widgets_embed_code'])) {
    $theme_registry['web_widgets_embed_code']['function'] = 'transition_pse_widget_web_widgets_embed_code';
  }
}

/**
 * Overrides theme_web_widgets_embed_code() to
 */
function transition_pse_widget_web_widgets_embed_code($code, $style) {
  $styles = web_widgets_get_styles();
  $style_name = $styles[$style];

  static $num = 0;
  $form = array(
    '#type' => 'textarea',
    '#title' => t('Embed code'),
    '#description' => t(
      'Copy and paste ALL of this code to your website. Please note that by embedding the widget into your site, you are agreeing to our <a href="!link">terms and conditions</a>.',
      array('!link' => url('terms'))
    ),
    '#id' => 'web_widgets_'. $num++,
    '#name' => 'web_widgets',
    '#value' => $code,
    '#parents' => array('none'),
    '#rows' => 10,
    '#resizable' => FALSE,
    '#attributes' => array('style' => 'width: 99%'),
    '#maxlength' => FALSE,
  );
  return drupal_render($form);
}

/**
 * Builds wrapper for PSE widget with all relevant information inserted.
 * Calls theme_pse_widget_view_container() and theme_pse_widget_entry to add view
 * and entry widget code.
 */
function theme_pse_widget_container($type, $widget_urls, $assets, $options) {
  // build wrapper and include assets
  $output = '<!-- TN-PSE: widget begin -->';
  $output .= '<script type="text/javascript">var tn_pse_widget_entry_url = "' . $widget_urls['entry'] . '"; var tn_pse_widget_about_url = "' . $widget_urls['about'] . '";</script>';
  $output .= '<script type="text/javascript" src="' . $assets['js'] . '"></script>';
  $output .= '<link rel="stylesheet" type="text/css" href="' . $assets['css'] . '" />';
  $output .= '<div class="tn-pse-widget-container tn-pse-widget-view-' . $type . '">';
  $output .= ' <div class="tn-pse-widget-header"></div>';

  // show view area if not button-only widget
  if ($type <> 'button') {
    // add view widget depending on $type
    $output .= theme('pse_widget_view_container', $type, $widget_urls, $options);
  }

  // add entry widget button and modal (hidden) dialog div
  $output .= theme('pse_widget_entry_container', $type, $widget_urls, $options);

  // finish up
  $output .= '</div><!-- TN-PSE: widget end -->';
  return $output;
}

/**
 * Builds View widget of correct $type and generates an iFrame containing
 * the contents of the appropriate view widget.
 */
function theme_pse_widget_view_container($type, $widget_urls, $options) {
  //generate view widget container and iframe
  $output = '<div class="widget-upper">';
  $output .= ' <iframe class="tn-pse-widget-view-widget" src="' . $widget_urls['view'] . '" frameborder="0"
      style="width: 100%; height: ' . $options['height'] . 'px; border: none; overflow: hidden;"></iframe>';
  $output .= '</div>';

  return $output;
}

/**
 * Builds Entry widget button & hidden modal dialogue including and iFrame
 * containing the contents of the entry widget.
 */
function theme_pse_widget_entry_container($type, $widget_urls, $options) {
  $output = '<div class="tn-pse-widget-widget-lower">';
  $output .= ' <div class="tn-pse-widget-grey-box"><p><strong>Are you involved in a community project? Want people to know?</strong></p>';
  $output .= '   <a href="' . $widget_urls['entry nojs'] . '" class="tn-pse-widget-entry-widget-launcher">';
  $output .= '    <span>' . $options['add button text'] . '</span></a>';

  // show 'about' link here if not full widget
  if ($type != 'full') {
    $output .= '   <p class="tn-pse-widget-about"><a href="' . $widget_urls['about'] . '" class="tn-pse-widget-about-widget-launcher">About this widget</a></p>';
  }
  $output .= ' </div>';
  $output .= '</div>';
  $output .= '<div class="tn-pse-widget-modal-mask" style="display: none">';
  $output .= ' <div class="tn-pse-widget-modal-outer">';
  $output .= '  <a href="#" class="tn-pse-widget-modal-button-close">Close</a>';
  $output .= '  <div class="tn-pse-widget-modal-inner"><!-- iFrame with class "entry-widget" will be inserted here --></div>';
  $output .= ' </div>';
  $output .= '</div>';
  return $output;
}


/**
 * Wraps widget interior
 */
function theme_pse_widget_view_inner($content) {
  return '<div class="tn-pse-widget-inner">' . $content . '</div>';
}

/**
 * Theme function for thank you page.
 */
function theme_pse_widget_entry_thanks() {
  $submit_another_url = '/pse/entry/add-project/' . arg(4) . '/' . arg(5);
  return theme('pse_widget_entry_thanks_content', $submit_another_url);

}