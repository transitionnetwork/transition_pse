<?php
/**
 * @file
 *
 * Include file for PSE widget generation and related tasks.
 */

/**
 * get widget code page callback
 * @todo tidy this up, break into smaller functions and theme()
 */
function transition_pse_widget_generate($account_uid = -1, $type = 'sidebar', $initiative_id = -1) {
  // belt & braces ensure user and type are ok
  global $user;
  if ($account_uid == -1) $account_uid = $user->uid;
  if ($type <> 'full' && $type <> 'button') $type = 'sidebar';
  $output = '';
  // set up options array for easy tweaking -- one day make overridable through UI
  $options = array(
    'add button text' => t('Add your project!'),
    'height' => 350 + ($type == 'full') * 250,
  );

  // check user has related initiative(s)
  $account = user_load($account_uid);
  $profile = content_profile_load('profile', $account->uid);
  $field = $profile->field_initiative;

  // prepare initiative data, load names, get nids from field
  $initiatives = array(
    'selected' => $initiative_id,
    'options' => NULL,
  );
  foreach ($field as $entry) {
    $initiatives['options'][$entry['nid']] = $entry['nid'];
  }

  // add in choice from querystring, if present
  if ($initiative_id <> -1 && !in_array($initiative_id, $initiatives['options'])) {
    $initiatives['options'][$initiative_id] = $initiative_id;
  }

  // load inis titles from nids, find match passed in if poss.
  $initiative_found = FALSE;
  foreach ($initiatives['options'] as $nid) {
    if ($node = node_load(array('nid' => $nid, 'status' => 1, 'type' => 'initiative_profile'))) {
      // ini loaded ok, add title to options
      $initiatives['options'][$nid] = $node->title;
      // note we've found the $initiative_id
      if ($nid == $initiative_id) $initiative_found = TRUE;
    }
    else {
      // can't load ini, remove option
      unset($initiatives['options'][$nid]);
    }
  }

  // warn & bail if no inis to chose from
  if (!count($initiatives['options'])) {
    $output .= t('<p><strong>Sorry, you cannot use the widget until you associate yourself with your local initiative</strong></p>');
    $output .= t(
      '<p>Please <a href="@link">edit your profile</a> and make sure your local initiative is selected in the <em>Initiatives involved in</em> field.</p>',
      array('@link' => '/user/' . $user->uid . '/edit/profile?destination=' . request_uri() . '#edit-field-initiative-nid-nid')
    );
    return $output;
  }

  // add form to allow choice of sidebar/full and ini to relate to via UI/tabs/links etc.
  $path = base_path() . drupal_get_path('module', 'transition_pse');
  $output .= '<div class="help">' . t(
    '<strong>What\'s the Project Sharing Widget?</strong> You might like to learn more about the widget and its use with our <a href="@link_faq">FAQs</a> and <a href="@link_setup">setup guide</a>.',
    array('@link_faq' => '/pse/widget/faq', '@link_setup' => url('/node/27590'))
  ) . '</div>';
  $output .= t('<h3>Step 1: Choose your style and initiative</h3>');
  $output .= t(
    '<p><img src="@path" alt="Screenshots of full sidebar widgets on initiative websites" class="float-right" height="400" width="440"> Remember to click "Update widget" if you make a change to these settings.<p>',
    array('@path' => $path . '/images/PSE_widget-screenshots.jpg')
  );
  $output .= drupal_get_form('transition_pse_widget_generate_options_form', $type, $initiatives, $options);


  if ($initiative_found) {
    // make pse urls, ensure secure
    global $base_url;
    $base_url_secure = str_replace('http:', 'https:', $base_url);
    $base_url_insecure = str_replace('https:', 'http:', $base_url);
    $widget_urls = array(
      'view' => $base_url_insecure . "/pse/view/$type/$account_uid/$initiative_id",
      'entry' => $base_url_secure . "/pse/entry/add-project/$account_uid/$initiative_id",
      'entry nojs' => $base_url_secure . "/pse/entry/add-project/$account_uid/$initiative_id",
    );
    // set up widget asset paths
    $assets = array(
      'js' => $base_url . '/' . $path . '/widget/widget.js',
      'css' => $base_url . '/' . $path . '/widget/widget.css',
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

    //@todo theme functions!
    $output .= t('<h3>4. Now the widget is on your website</h3>');

    $output .= t('<p>To get projects added you now need to promote the widget: Put it in your
      newsletters, email-outs, link to it from your news items, and tell everyone about it.</p>');
    $output .= t(
      '<p>Once people start adding their proejcts, you will get the project submissions alerts
      via email. Please attend to these as soon as you can; it won\'t take long. You will be able to moderate
      tje submissions in the <a href="@link"><em>Projects moderation</em> tab </a>of your user account.</p>',
      array('@link' => 'user/' . $account_uid . '/widget-moderation')
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
function transition_pse_widget_generate_options_form($form, $type, $initiatives, $options) {
  global $user;
  $form = array();
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
    '#description' => t(
      'Choose the related Transition Initiative for this widget so that local projects can be shown.<br/><strong>Tip:</strong>
       you can add to the list of initiatives by <a href="@link">editing your profile</a> and selecting them in the
       <em>Initiatives involved in</em> field.',
      array('@link' => '/user/' . $user->uid . '/edit/profile?destination=' . request_uri() . '#edit-field-initiative-nid-nid')
    ),
    '#default_value' => $initiatives['selected'],
    '#options' => $initiatives['options'],
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update widget'));
  return $form;
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
    '#description' => t('Copy and paste ALL this code to your website.'),
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
 * submit handler for transition_pse_widget_generate_options_form
 */
function transition_pse_widget_generate_options_form_submit($form, &$form_state) {
  global $user;
  $path = 'pse/widget/get/' . $user->uid . '/' . $form_state['values']['type'] . '/' . $form_state['values']['initiative'];
  drupal_goto($path);
}

/**
 * Builds wrapper for PSE widget with all relevant information inserted.
 * Calls theme_pse_widget_view_container() and theme_pse_widget_entry to add view
 * and entry widget code.
 */
function theme_pse_widget_container($type, $widget_urls, $assets, $options) {
  // build wrapper and include assets
  $output = '<!-- TN-PSE: widget begin -->';
  $output .= '<script type="text/javascript">var tn_pse_widget_entry_url = "' . $widget_urls['entry'] . '";</script>';
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
  $output .= theme('pse_widget_entry_container', $widget_urls, $options);

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
function theme_pse_widget_entry_container($widget_urls, $options) {
  $output = '<div class="tn-pse-widget-widget-lower">';
  $output .= ' <div class="tn-pse-widget-grey-box"><p><strong>Are you involved in a community project? Want people to know?</strong></p>';
  $output .= '   <a href="' . $widget_urls['entry nojs'] . '" class="tn-pse-widget-entry-widget-launcher">';
  $output .= '    <span>' . $options['add button text'] . '</span></a>';
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
 *
 * @TODO: move this waffle to a .tpl.php file
 */
function theme_pse_widget_entry_thanks() {
  $output = '<h3>Thank you for adding your project</h3>';
  $output .= '<p>We appreciate you taking the time to share what you are doing.</p>';
  $output .= '<p>You will receive an email confirmation about this project, as will the webmaster of the website you are visiting. Your project ';
  $output .= 'has been added as \'not live\' to your local webmaster\'s \'moderation\' queue, and they will take a look and approve it as soon ';
  $output .= 'as possible. You will will receive an email confirmation about the webmaster\'s approval when it happens. You will also receive an ';
  $output .= 'email from Transition Network asking you to confirm your user account. You need to confirm you account in order to make any edits ';
  $output .= 'later, so please read this and confirm your account now.</p>';
  $output .= '<p>If you do not hear anything, please contact your local webmaster asking them to check their projects list, or ';
  $output .= '<a href="mailto:webproject@transitionnetwork.org">the Transition Network web project</a> asking what has happened.</p>';

  $output .= '<h4>Got another project?</h4><p> We\'d love to hear about it, please <a href="/pse/entry/add-project/' . arg(4) . '/' . arg(5) . '">add your other project</a> now too!</p>';

  $output .= '<h4>Finished</h4><p>You can now close this window and return to the local website you are visiting or you can go and ';
  $output .= '<a href="http://www.transitionnetwork.org/projects" target="_blank" title="visit the full Projects Directory of community ';
  $output .= 'projects from across the world on the Transition Network website">other projects in the Transition Network Projects Directory</a>.</p>';
  return $output;
}

/**
 * Theme function for about details.
 *
 * @TODO: move this waffle to a .tpl.php file
 */
function theme_pse_widget_view_about() {
  $output = '<h3>Project Sharing Engine Alpha</h3><h4>What?</h4>';
  $output .= '<p>The <strong>Project Sharing Engine</strong> is a service to present Transition projects from around the world on Transition ';
  $output .= 'Initiatives\' own websites. It also enables people to add information about their own projects to the Transition Network projects ';
  $output .= 'directory from Transition Initiatives\' own websites, without having to visit the Transition Network website.</p>';
  $output .= '<h4>How?</h4>';
  $output .= '<p>It is all done with a simple widget which can be placed onto a Transition Initiative website by that website\'s webmaster. ';
  $output .= 'Then a visitor to that website can use the widgets to browse projects in the area, or around the world, and add their own. The ';
  $output .= 'webmaster can then approve the project, making it live in the Transition Network\'s main Projects Directory. We want it to be ';
  $output .= 'simple for the Transition Initiative webmaster to add a widget to their site, and we want it to be simple for a Transition ';
  $output .= 'Initiative website visitor to add their project in a few minutes. </p>';
  $output .= '<p>We think that\'s done. We are working with a community of \'Transition webmasters\' to help develop the service. They are called ';
  $output .= 'the \'alpha web masters\' as they have agreed to try the early version of the widget which you are looking at now.</p>';
  $output .= '<h4>Why?</h4><p>Community activists around the world are doing their best to do great projects and we want to help. We want to help ';
  $output .= 'projects share knowledge about, and contacts for, community projects around the world, as easily and simply as possible. We don\'t ';
  $output .= 'want people to have to come to our website, we want them to stay on their own local sites and share their knowledge from there. So ';
  $output .= 'they can spend more time locally. We also hope that this will provide Transition Initiatives\' websites with inspiring content from ';
  $output .= 'around and beyond the Transition movement, thereby making their sites even better.</p>';
  return $output;
}
