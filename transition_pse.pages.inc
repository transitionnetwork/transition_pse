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
  if ($_SERVER['HTTP_HOST'] == 'tn.dev') {
    // JK make it erroneous locally!
    //@todo remove after Alpha/beta test phase
    error_reporting(E_ALL);
    ini_set('display_errors', TRUE);
    ini_set('display_startup_errors', TRUE);
  }
  // hide admin nmenu
  if (module_exists('admin_menu')) {
    admin_menu_suppress();
  }

  // check the participant is valid, bail if not.
  $participants = _transition_pse_participants_load($participant_uid, $initiative_nid);
  if (isset($participants['errors'])) return _transition_pse_invalid_participants_warning($participants['errors']);

  // build nicely themed page
  $output = '';
  // check user is logged in
  global $user;
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

  // Website @todo should use Link module, but can't find out how to show only URL!
  // Damn crappy Drupal contrib documentation! Have to map to correct field now, and
  // validate it later with Link module's link_validate_url().
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

  // add MailChimp list option, if available
  // @see Adapted from mailchimp_user() 'register' section
  /*
  if (module_exists('mailchimp')) {
    global $user;
    $lists = _mailchimp_get_available_lists($user);
    // wrap in a fieldset
    $form['mailchimp_list_forms'] = array(
      '#type' => 'fieldset',
      '#title' => t('Keep me updated about the widget'),
      '#weight' => 8,
    );
    // get MC subscribe form, subscribe by default
    $mcform = mailchimp_subscribe_auth_form($form_state, $user, FALSE, $lists[TRANSITION_PSE_MC_LID]);
    $mcform['wrapper' . TRANSITION_PSE_MC_LID]['mailchimp_list_' . TRANSITION_PSE_MC_LID]['#default_value'] = TRUE;
    // merge into our form
    $form['mailchimp_list_forms'][] = $mcform['wrapper' . TRANSITION_PSE_MC_LID];
    unset($mcform['wrapper' . TRANSITION_PSE_MC_LID]);
    $form = array_merge($form, $mcform);
  }
  */

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
  if (module_exists('link') && !empty($website)) {
    // validate link href with Link module function (this module has a dependency on Link).
    // Smelly but effective.
    $type = link_validate_url($website);
    // only allow external links
    if ($type != LINK_EXTERNAL) {
      form_set_error('website', t('Not a valid external website URL.'));
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

  // subscribe user to mailchimp list
  // @see Adapted from mailchimp_user() 'insert' section
  /*
   * if (module_exists('mailchimp') && $q = _mailchimp_get_api_object()) {
    foreach ((array)$form_state['values']['lists'] as $list) {
      // is the checkbox for the newsletter selected?
      if (isset($form_state['values']['mailchimp_list_' . $list->id] ) && $form_state['values']['mailchimp_list_' . $list->id] ) {
        $merge_vars = _mailchimp_load_user_list_mergevars($user->uid, $list->id, $q->listMergeVars($list->id));
        $ret = _mailchimp_subscribe_user($list, $user->mail, $merge_vars, TRUE, $q);
        if (!$ret) {
          watchdog('mailchimp', 'MCAPI Error: %errormsg', array('%errormsg' => $q->errorMessage), WATCHDOG_ERROR);
        }
      }
    }
  }
  * */
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
    'title' => $initiative_title,
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

/**
 * Creates the $message for a submitting user an accept or reject message, based
 * on $params['node'] and $params['author'].
 */
function transition_pse_submitter_mail_text($type, &$message, $params) {
  // get language, add standard start to message
  $langcode = isset($message['language']) ? $message['language']->language : NULL;
  $body = t("Dear @author_name,\n\n", array('@author_name' => $params['author']->name), $langcode);

  switch ($type) {
    // thank you body
    case 'thanks':
      $message['subject'] = t("Thank you for your project submission!", array(), $language);
      $body .= t("Thank you for adding your community project through the Transition Network projects widget. We appreciate you taking the time to share what you know about what you are up to.\n\n", array(), $langcode);
      $body .= t(
        "Your project, @node_title, is not published yet; the webmaster of the Transition Initiative website hosting the widget needs to give it a quick check before it is published. ",
        array(
          '@node_title' => $params['node']->title,
        ),
        $language
      );
      $body .= t("The webmaster has received an email asking them to do this. Once they have checked it, you will receive an email confirmation letting you know whether it has been approved or not.\n\n", array(), $langcode);
      $body .= t("We are confident that your project is good, but you can imagine that we need to check things that come into the directory to make sure that they are not spam. Your patience is appreciated.\n\n", array(), $langcode);
      break;

    // accepted message body
    case 'accepted':
      $message['subject'] = t("Good news about your project submitted via the Transition Network widget!", array(), $language);
      $body .= t("You recently added a community project through the 'Project Sharing Engine' widget. We are pleased to say that the webmaster of the participating site has approved your project and it is now in the main Transition Network Projects Directory.\n\n", array(), $langcode);
      $body .= t(
        "Project: @node_title\nLink: @node_link\n\nYou can edit this at any time -- please do! Please add some more information with details of the goals, outcomes, lessons etc. It all goes to making someone else's project learn from your experience.\n\nYou can log into the Transition Network site from your project page by clicking the 'My Account & Login' link in the top right corner of the page. REMEMBER: you need to have confirmed your email address to edit your project profile.\n\nHere is the link to your project profile again: @node_link\n\n",
        array(
          '@node_title' => $params['node']->title,
          '@node_link' => url('node/' . $params['node']->nid, array('absolute' => TRUE)),
        ),
        $langcode
      );
      break;

    // rejected message body
    case 'rejected':
      $message['subject'] = t("Your project submission to the Transition Network widget", array(), $language);
      $body .= t("You added a community project through the 'Project Sharing Engine' widget on a participating website. We are sorry to say that it has not been added to the Transition Network Projects Directory.\n\nIf you have any queries about this, please contact the Transition Network web project at webproject@transitionnetwork.org.\n\n", array(), $language);
      break;

  }
  // finish up
  $body .= t("Thank you and all the best,\n\nThe web team at the Transition Web Project.", array(), $langcode);
  $message['body'][] = $body;
}

/**
 * Sends a webmaster (widget owner) notification of new submission, or a
 * reminder, based on $params['node'] and $params['webmaster'].
 */
function transition_pse_webmaster_mail_text($type, &$message, $params) {
    // get language, add standard start to message
  $langcode = isset($message['language']) ? $message['language']->language : NULL;
  $body = t("Hi @author_name,\n\n", array('@author_name' => $params['webmaster']->name), $langcode);
  // new message body
  if ($type == 'new') {
    $message['subject'] = t("New project submission via your PSE widget", array(), $language);
    $body .= t("Someone has added a project through the widget on your website. Please follow the moderation link below, check the submission and approve it if it is suitable. If it is not suitable, please do reject it! This will take less than two minutes.\n\n", array(), $langcode);
  }
  // reminder message body
  if ($type == 'reminder') {
    $message['subject'] = t("Reminder of project submission via your PSE widget", array(), $language);
    $body .= t("Someone added a project through the widget on your website one week ago, and we don't think that you have checked it yet. Please can you take 2 minutes to check it?\n\nPlease follow the link below, check the submission and approve it if it is suitable. If it is not suitable, please do reject it!\n\n", array(), $language);
  }

  // add info & links
  $body .= t(
    "Project: @node_title\nModeration: @moderation_link\nLog in: @login_link\n\n",
    array(
      '@node_title' => $params['node']->title,
      '@moderation_link' => url('user/' . $params['webmaster']->uid . '/widget-moderation', array('absolute' => TRUE)),
      '@login_link' => url('user/login', array('absolute' => TRUE)),
    ),
    $langcode
  );
  // closing gambit
  $body .= t("Thanks again for hosting a Transition Projects Widget, and taking the time to check the project.\n\nThank you and all the best,\n\nThe web team at the Transition Web Project.", array(), $langcode);
  $message['body'][] = $body;
}