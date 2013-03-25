<?php
/**
 * @file
 *
 * Transition Project Sharing Engine pages include -- various user interface
 * and email functions for Widget management
 */

/**
 * Builds and modifies the Project entry form
 */
function transition_pse_entry_form_steps($participant_uid = 0, $initiative_nid = 0) {
  // hide admin nmenu
  if (module_exists('admin_menu')) {
    admin_menu_suppress();
  }

  // check the participant is valid, bail if not.
  $participants = _transition_pse_participants_load($participant_uid, $initiative_nid);
  if (isset($participants['errors'])) return _transition_pse_invalid_participants_warning($participants['errors']);

  // build nicely themed page
  $output = '';
  global $user;

  // check user is logged in
  if ($user->uid) {
    // we're logged in, woop, show project add form
    $output .= transition_pse_entry_page($participants);
  }
  else {
    // not logged in, show user login/registration page
    $output .= transition_pse_unified_login_page($participants);
  }
  return theme('pse_widget_page', $output);
}

/**
 * Builds and modifies the Project entry form
 */
function transition_pse_entry_page($participants) {
  _transition_pse_overriding_form_flag(TRUE);
  return drupal_get_form('transition_pse_entry_form');
}

/**
 * Form generation for PSE entry widget
 */
function transition_pse_entry_form($form_state) {
  $form = array();
  module_load_include('inc', 'maxlength');
  // Title
  $form['title'] = array(
    '#type' => 'textfield',
    '#required' => '1',
    '#title' => t('Project title'),
    '#size' => '75',
    '#weight' => 0,
  );

  // Summary
  $form['summary'] = array(
    '#type' => 'textarea',
    '#required' => '1',
    '#title' => t('Please add a short summary of your project'),
    '#weight' => 1,
  );
  // Adds MaxLength stuff by cheating and referring to Project CT field_project_summary settings.
  // Unfortunately D6 versions is fucking DREADFUL so you can't just apply to any old field without
  // dicking around a bunch.
  if (module_exists('maxlength')) {
    $form['summary']['#max_length_properties'] = array($form['summary']['#default_value'], 'field_project_summary', 'summary', 'project');
    $form['summary']['#after_build'] = array('_maxlength_format_element');
  }

  // Simple website field
  $form['website'] = array(
    '#type' => 'textfield',
    '#title' => t('Project website'),
    '#weight' => 2,
  );

  // Location
  $form['locations'] = array(
    '#type' => 'location_element',
    '#required' => '1',
    '#title' => t('Location'),
    '#size' => '75',
    '#weight' => 5,
    '#location_settings' => array(
      'form' => array(
        'fields' => array(
          'country' => array('collect' => 2, 'default' => 'uk', 'weight' => 0),
          'province' => array('collect' => 1, 'widget' => 'select', 'weight' => 1),
          'city' => array('collect' => 2, 'weight' => 2),
          'postal_code' => array('collect' => 2),
          'name' => array('collect' => 0),
          'street' => array('collect' => 0),
          'locpick' => array('collect' => 0),
        ),
      ),
    )
  );

  // Transition project? getting data from CCK field in an attempt to keep things
  // auto synced... But could prob hardcode if easier...
  $field = content_fields('field_project_org_type', TRANSITION_PSE_CT);
  $options = array();
  foreach (explode("\n", $field['allowed_values']) as $option) {
    $option = explode('|', $option);
    $options[$option[0]] = $option[1];
  }
  // override text on 'Transition' projects/
  $options['Transition'] = t('Transition project (directly related to a Transition Initiative)');
  $form['project_type'] = array(
    '#type' => 'radios',
    '#title' => t(check_plain($field['widget']['label'])),
    '#default_value' => $field['widget']['default_value'][0]['value'],
    '#options' => $options ,
    '#weight' => 7,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit your project!'),
    '#weight' => 10,
  );
  return $form;
}

/**
 * Form validation for PSE entry widget
 */
function transition_pse_entry_form_validate($form, &$form_state) {
  // extra validation for link here
  $website = $form_state['values']['website'];
  if (!empty($website)) {
    // validate link href with PHP 5.2+ Link, and then check it is http(s)
    if (filter_var($website, FILTER_VALIDATE_URL) === false || strpos($website, 'http') !== 0) {
      form_set_error('website', t('Not a valid external website URL - must begin with lowercase \'http://\' or \'https://\'.'));
    }
  }
}

/**
 * Form submit handler for PSE entry widget
 */
function transition_pse_entry_form_submit($form, &$form_state) {
  // get user and ini participants for this widget request
  $participants = _transition_pse_participants_load(arg(3), arg(4));

  // build new Project node, add values
  global $user;
  $node = new stdClass();
  $node->type = TRANSITION_PSE_CT;
  $node->uid = $user->uid;
  $node->status = 1; //(1 or 0): published or not
  // title & body
  $node->title = $form_state['values']['title'];
  $node->body = $form_state['values']['summary'];
  // location
  $location = array(
    'postal_code' => $form_state['values']['locations']['postal_code'],
    'city' => $form_state['values']['locations']['city'],
    'province' => $form_state['values']['locations']['province'],
    'country' => $form_state['values']['locations']['country'],
    'is_primary' => 1,
  );
  // save new location first, then connect its $new_lid to our node
  $new_lid = location_save($location);
  $node->locations[0]['lid'] = $new_lid;
  // project type
  $node->field_project_org_type[0]['value'] = $form_state['values']['project_type'];
  // set related initiative as releated and web contact to widget owner
  $node->field_submission_initiative[0]['nid'] = $participants['initiative']->nid;
  $node->field_submission_webmaster[0]['uid'] = $participants['account']->uid;
  // website
  $node->field_website_home[0]['url'] = $form_state['values']['website'];

  // Save this node and avoid this sending notifications
  $node->notifications_content_disable = 1;
  node_save($node);

  // if saved ok, send email notifications (would love D7 Rules here... ;-) )
  if ($node->nid) {
    // tell webmaster about the new submission
    $params = array(
      'node' => $node,
      'webmaster' => $participants['account'],
    );
    drupal_mail('transition_pse', 'webmaster_submission_new', $participants['account']->mail, language_default(), $params);

    // thank this user
    unset($params['webmaster']);
    $params['author'] = $user;
    drupal_mail('transition_pse', 'submitter_project_thanks', $user->mail, language_default(), $params);

    // and redirect to the thank you page when done
    $redirect_to = 'pse/entry/add-project/thank-you/' . arg(3) . '/' . arg(4) . '?done=' . uniqid();
    $_REQUEST['destination'] = $redirect_to;
  }
}


/**
 * Mailchimp signup form for Widget Newsletter
 *
 * @see Adapted from mailchimp_user() 'register' section
 */
function transition_pse_newsletter_form($form_state, $stand_alone = FALSE) {
  if (module_exists('mailchimp')) {
    global $user;
    $lists = _mailchimp_get_available_lists($user);
    // get MC subscribe form
    $form = mailchimp_subscribe_auth_form($form_state, $user, FALSE, $lists[TRANSITION_PSE_MC_LID]);

    // if we have found our newsletter
    if (isset($form['wrapper' . TRANSITION_PSE_MC_LID])) {
      // subscribe by default and add submit button, return form
      $form['wrapper' . TRANSITION_PSE_MC_LID]['mailchimp_list_' . TRANSITION_PSE_MC_LID]['#title'] = t('Please keep me up to date with changes to widget code and other related updates (no spam, we promise)!');
      $form['wrapper' . TRANSITION_PSE_MC_LID]['mailchimp_list_' . TRANSITION_PSE_MC_LID]['#default_value'] = TRUE;
      $form['newsletter-note'] = array(
        '#type' => 'markup',
        '#value' => t('<p><strong>Note:</strong> The widget\'s code will change so please don\'t opt out of this - we will need to update you occasionally.</p>'),
      );
      // add submit button if needed
      if ($stand_alone) {
        $form['submit'] = array(
          '#type' => 'submit',
          '#value' => t('Get the newsletter'),
        );
      }
      return $form;
    }
  }
  else {
    watchdog('transition_pse', 'Mailchimp module not available');
  }
}

/**
 * Submit handler for Mailchimp Newsletter signup -- won't allow unsubscribe (by design!)
 *
 * see Adapted from mailchimp_user() 'insert' section
 */
function transition_pse_newsletter_form_submit($form, &$form_state) {
  global $user;
  if (module_exists('mailchimp') && $q = _mailchimp_get_api_object()) {
    foreach ((array)$form_state['values']['lists'] as $list) {
      // is the checkbox for the newsletter selected?
      if (isset($form_state['values']['mailchimp_list_' . $list->id]) && $form_state['values']['mailchimp_list_' . $list->id]) {
        // subscribe
        $merge_vars = _mailchimp_load_user_list_mergevars($user->uid, $list->id, $q->listMergeVars($list->id));
        $ret = _mailchimp_subscribe_user($list, $user->mail, $merge_vars, TRUE, $q);
        if (!$ret) {
          watchdog('mailchimp', 'MCAPI Error: %errormsg', array('%errormsg' => $q->errorMessage), WATCHDOG_ERROR);
        }
        else {
          drupal_set_message('Thank you for subscribing.');
        }
      }
    }
  }
}
/**
 * Builds a unified login/registration page
 *
 * adapted from Login Toboggan patch at:
 *  http://drupal.org/files/issues/logintoboggan-264332-15.patch
 */
function transition_pse_unified_login_page($participants) {
  // TODO: Somehow add a class of "default" to whichever form is the default.
  // Then, in the JS, remove hard-coding of hiding the register form, and
  // instead hide the one not marked 'default'
  $active_form = 'register';

  // get cleaned user reg/login forms, set destination
  _transition_pse_overriding_form_flag(TRUE);
  $login_form = drupal_get_form('user_login');
  $register_form = drupal_get_form('user_register');

  // Add some JS and CSS loveliness, build output
  $path = drupal_get_path('module', 'transition_pse');
  drupal_add_css($path . '/css/entry.css');
  drupal_add_js($path . '/js/unified-login-registration.js');
  drupal_add_js(array('lt' => array('activeForm' => $active_form)), 'setting');
  $output = '<div class="pse-unified ' . $active_form . '">';
  // Create the initial message and links that people can click on.
  $output .= '<div id="login-message">' . t('Please log in using your Transition Network user account, or register to tell us who you are.') . '</div>';
  $output .= '<div id="login-links" class="clear-block">';
  $output .= l(t('Quickly create an account'), 'user/register', array('attributes' => array('class' => 'login-link', 'id' => 'register-link')));
  $output .= l(t('Use my Transition Network account'), 'user/login', array('attributes' => array('class' => 'login-link', 'id' => 'login-link')));
  $output .= '</div>';
  // Add the login and registration forms in.
  $output .= '<div id="login-form">';
  $output .= $login_form;
  $output .= '</div>';
  $output .= '<div id="register-form">' . $register_form . '</div>';
  $output .= '</div>';
  return $output;
}

/**
 * Page callback for 'Thank you' page
 */
function transition_pse_entry_thanks() {
    // hide admin menu, call theme function
  if (module_exists('admin_menu')) admin_menu_suppress();
  // drupal_set_title('Step 3: Thank you!');
  $output = theme('pse_widget_entry_thanks');
  return theme('pse_widget_page', $output);
}


/**
 * the callback for the sidebar widget page
 */
function transition_pse_view_sidebar($participant_uid = 0, $initiative_nid = 0) {
  // check the participant is valid, bail if not.
  $participants = _transition_pse_participants_load($participant_uid, $initiative_nid);
  if (isset($participants['errors'])) return _transition_pse_invalid_participants_warning($participants['errors']);
  drupal_set_title('Community projects near ' . check_plain($participants['initiative']->title));
  // get coordinates from initiative
  $coord_arg = _transition_pse_prepare_location_argument($participants);
  // load views module and execute
  // hide admin nmenu
  if (module_exists('admin_menu')) {
    admin_menu_suppress();
  }
  return theme(
    'pse_widget_view_inner',
    views_embed_view(
      'transition_pse_widget',
      'block_1',
      $coord_arg
    )
  );
}

/**
 * the callback for the full widget page
 */
function transition_pse_view_full($participant_uid = 0, $initiative_nid = 0) {
  // check the participant is valid, bail if not.
  $participants = _transition_pse_participants_load($participant_uid, $initiative_nid);
  if (isset($participants['errors'])) return _transition_pse_invalid_participants_warning($participants['errors']);
  $initiative_title = check_plain($participants['initiative']->title);
  drupal_set_title('Community projects near ' . $initiative_title);

  // get coordinates from initiative
  $coord_arg = _transition_pse_prepare_location_argument($participants);

  // make quicktabs
  $tabs = array();
  $tabs['initiative'] = array(
    'title' => 'Near initiative',
    'type' => 'view',
    'vid' => 'transition_pse_widget',
    'display' => 'block_2',
    'args' =>   $coord_arg,
  );
  $tabs['search'] = array(
    'title' => t('Find projects'),
    'type' => 'callback',
    'path' => 'pse/view/full/search',
  );
  $tabs['info'] = array(
    'title' => t('About'),
    'type' => 'callback',
    'path' => 'pse/view/about',
  );
  $quicktabs['qtid'] = 'tn-pse-view-full-tabs';
  $quicktabs['tabs'] = $tabs;
  $quicktabs['ajax'] = TRUE;
  $quicktabs['hide title'] = TRUE;
  // hide admin nmenu
  if (module_exists('admin_menu')) {
    admin_menu_suppress();
  }
  // wrap tabs in wiget inner, allow theme override
  return theme('pse_widget_view_inner', theme('quicktabs', $quicktabs));
}

/**
 * Menu callback for 'Find projects' quicktab on full widget
 */
function transition_pse_view_full_search() {
  // get view, tweak for AJAX, render
  $view = views_get_view('transition_pse_widget');
  //$view->override_path = $_GET['q'];
  $output = $view->execute_display('block_3');
  return $output['content'];
}

/**
 * Menu callback for 'About' quicktab on full widget
 */
function transition_pse_view_about() {
  return theme('pse_widget_view_about');
}

/**
 * Returns latitude/longitude and radius for view argument in format
 * "lat,long_radius". Note, lat/long are rounded to to 2 decimal places to
 * improve chances of views caching.
 */
function _transition_pse_prepare_location_argument($participants) {
  $latitude = round($participants['initiative']->location['latitude'], 2);
  $longitude = round($participants['initiative']->location['longitude'], 2);
  return $latitude . ',' . $longitude . '_' . TRANSITION_PSE_RADIUS;
}

/**
 * auto-remove non-manadatory fields from FAPI array recursively
 */
function _transition_pse_hide_non_manadatory_elements(&$elements) {
  $mandarory_count = 0;
  foreach ($elements as $key => $data) {
    if (substr($key, 0, 6) == 'field_') {
      if (!isset($data['#required']) || $data['#required'] != TRUE) {
        $elements[$key]['#access'] = FALSE;
      }
      else {
        $mandarory_count++;
      }
    }
    elseif (substr($key, 0, 6) == 'group_') {
        // recursively hide for groups
        if (_transition_pse_hide_non_manadatory_elements($elements[$key]) == 0) {
          // nothing left to show in this group so hide it
          $elements[$key]['#access'] = FALSE;
        }
      }
  }
  return $mandarory_count;
}


/**
 * shows wry $error text when a widget fails
 */
function _transition_pse_invalid_participants_warning($error) {  // hide admin nmenu
  if (module_exists('admin_menu')) {
    admin_menu_suppress();
  }
  drupal_set_title("This is not the widget you are looking for...");
  return t(
    "<h3>There's a widgety bug in your didgeridoo, mate</h3>
    <p>Please check your widget settings because this error has occurred: %error</p>",
    array('%error' => $error)
  );
}

/**
 * checks particpating site is ok
 *
 * @return  A valid User ID of the related participant, or FALSE if invalid.
 */
function _transition_pse_participants_load($participant_uid, $initiative_nid) {
  // ensure plain text
  $participant_uid = check_plain($participant_uid);
  $initiative_nid = check_plain($initiative_nid);
  $participants = array();
  // check initiative nid argument and is a number > 0
  if (!is_numeric($initiative_nid) || $initiative_nid == 0) {
    $participants['errors'] = t('Invalid participating initiative ID');
  }
  else {
    // basic validation ok, load ini node if type and published status match
    $initiative = node_load(array('nid' => $initiative_nid, 'status' => 1, 'type' => 'initiative_profile'));
    if ($initiative === FALSE) $participants['errors'] = t("Can't find or use chosen participating initiative");
    $participants['initiative'] = $initiative;
  }
  // check account nid argument and is a number > 0
  if (!is_numeric($participant_uid) || $participant_uid == 0) {
      $participants['errors'] = t('Invalid participating account ID');
  }
  else {
    // basic validation ok, load ini node if type and published status match
    $account = user_load($participant_uid);
    if ($account === FALSE) $participants['errors'] = t("Can't find participating user account");
    $participants['account'] = $account;
  }
  return $participants;
}
