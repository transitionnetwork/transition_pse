<?php
/**
 * @file
 *
 * Transition Project Sharing Engine pages include -- various user interface
 * and email functions for Widget management
 */

/**
 * Implementation of hook_form_alter()
 *
 * when _transition_pse_overriding_form_flag is set, this removes all
 * non-mandatory items from the Project forms.
 *
 * Also ensures all widet-submitted content is flagged for moderation
 */

function transition_pse_form_alter(&$form, &$form_state, $form_id) {
  if (_transition_pse_overriding_form_flag() == TRUE) {
    switch ($form_id) {
      case "user_register":
        _transition_pse_hide_non_manadatory_elements($form);

        // hide username since it will be auto-created from first/last name on user save
        // but for now make it unique to get through safely
        $form['account']['name']['#type'] = 'hidden';
        $form['account']['name']['#default_value'] = 'PSEUSER--' . uniqid();
        unset($form['account']['pass']['#type']);

        // email tweak message
        $form['account']['mail']['#description'] = t('We will not use your email address for anything else, nor give it to anyone else. ') . $form['account']['mail']['#description'];

        // hide unneeded newsletter
        // @BUG see: https://tech.transitionnetwork.org/trac/ticket/447
        $form['mailchimp_list_forms']['wrapper1b720da06c']['#access'] = FALSE;

        // stop LoginToboggan doing its stuff
        $logintoboggan_index = array_search('logintoboggan_user_register_validate', $form['#validate']);
        if ($logintoboggan_user_login_validate !== FALSE) {
          unset($form['#validate'][$logintoboggan_index]);
        }

        // ensure we can build username on submit
        array_unshift($form['#submit'], 'transition_pse_user_register_submit');

        // remove field_roles_offered, field_initiative and field_themes from
        // display/validation
        $form['group_user_interest']['field_roles_offered']['#type'] = 'optionwidgets_select';
        $form['group_user_interest']['field_roles_offered']['#access'] = FALSE;
        unset($form['group_user_interest']['field_initiative']);
        unset($form['group_user_interest']['field_themes']);

        // and redirect to the thank you page when done
        $redirect_to = 'pse/entry/add-project' . '?login=' . uniqid();
        $_REQUEST['destination'] = $redirect_to;

        /*@todo change message on register:
         * A validation e-mail has been sent to your e-mail address. In
         * order to gain full access to the site, you will need to follow
         * the instructions in that message.
         *
         * Login successful
         *
         * to something like:
         * Thanks for registering! Please enter your project details below.
         * You've also been sent a message asking you to confirm your email
         * address, but we'd rather hear about your project right now!
         */
        break;
    }
  }
}

/**
 * User registraton submit callback
 *
 * Builds a unique username based on first & last name, avoiding collisions
 */
function transition_pse_user_register_submit($form, &$form_state) {
  // build username from first & last names
  $first_name = $form_state['content_profile_registration']['profile']['node']->field_name_first[0]['value'];
  $last_name .= $form_state['content_profile_registration']['profile']['node']->field_name_last[0]['value'];
  // make alphanumeric only, set proper noun capitalisation
  $full_name = preg_replace("/[^a-zA-Z0-9]+/", "", $full_name);
  $full_name = ucwords(strtolower($first_name . ' ' . $last_name));
  // then ensure not too long after making space for 'counter' on end
  if (strlen($full_name) >= USERNAME_MAX_LENGTH - 3) $name = substr($name, 0, USERNAME_MAX_LENGTH - 3);

  // ensure username is unique
  $attempt = 0;
  $append = '';
  while (!$suceess && $attempt < 256) {
    // add attempt number to end of username if not unique - e.g. "John Smith 2"
    if ($attempt > 0) $append = ' ' . dechex($attempt);

    $result = db_query("SELECT name FROM {users} u WHERE u.name = '%s'", $full_name . $append);
    if (db_fetch_array($result) === FALSE) {
      // found a unique name, use it!
      $form_state['values']['name'] = $full_name . $append;
      break;
    }
    // otherwise carry on up to 0xff appended to name... or 256 chances.
    $attempt++;
  }
}

/**
 * Implementation of hook_user()
 *
 * Ensures PSE registration/login submits go back to entry PSE form.
 * Also builds a username from first and last name elements for PSE.
 */
function transition_pse_user($op, &$edit, &$account, $category = NULL) {
  // only for PSE entry widget
  if (arg(0) <> 'pse' || arg(1) <> 'entry') return;

  switch ($op) {
    case 'login':
    case 'register':
      // redirect login/form to itself for PSE only
      // build url with hex timestamp to minise caching issues
      $_REQUEST['destination'] = request_uri() . '?' . $op . '=' . uniqid();
      break;
  }
}

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
function transition_pse_entry_form() {
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

  // Aim
  $form['summary'] = array(
    '#type' => 'textarea',
    '#required' => '1',
    '#title' => t('What is this project about?'),
    '#description' => t('Provide a brief statement that describes the aim and background to your project.'),
    '#weight' => 1,
    '#max_length_properties' => array(
      'limit' => 300,
      'use_js' => TRUE,
      'text' => t("!remaining of !limit characters remaining."),
    ),
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
          'name' => array('collect' => 0),
          'street' => array('collect' => 0),
          'locpick' => array('collect' => 0),
          'city' => array('collect' => 2),
          'postal_code' => array('collect' => 2),
          'province' => array('collect' => 1, 'widget' => 'autocomplete'),
          'country' => array('collect' => 2, 'default' => 'uk'),
        ),
      ),
    )
  );

  // Transition project? get data from CCK field
  $field = content_fields('field_project_org_type', TRANSITION_PSE_CT);
  $options = array();
  foreach (explode("\n", $field['allowed_values']) as $option) {
    $option = explode('|', $option);
    $options[$option[0]] = $option[1];
  }
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
  //@todo add extra validation as it comes in here
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
  //@todo CHECK these may be bad assumptions!
  $node->field_submission_initiative[0]['nid'] = $participants['initiative']->nid;
  $node->field_submission_webmaster[0]['uid'] = $participants['account']->uid;

  // Save this node and avoid this sending notifications
  $node->notifications_content_disable = 1;
  node_save($node);

  // if saved ok, send notifications
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
    $redirect_to = 'pse/entry/add-project/thank-you' . '?done=' . uniqid();
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
    'path' => 'pse/view/full/about',
  );
  $quicktabs['qtid'] = 'tn-pse-view-full-tabs';
  $quicktabs['tabs'] = $tabs;
  $quicktabs['ajax'] = TRUE;
  $quicktabs['hide title'] = TRUE;

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
function transition_pse_view_full_about() {
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
        "Project: @node_title\nLink: @node_link\n\nYou can edit this at any time -- please do! Please add some more information with details of the goals, outcomes, lessons etc. It all goes to making someone else's project learn from your experience.\n\nYou can log into the Transition Network site from your project page by clicking the 'My Account & Login' link in the top right corner of the page.\n\nHere is the link to your project profile again: @node_link\n\n",
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