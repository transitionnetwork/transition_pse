<?php
/**
 * @file
 *
 * Transition Project Sharing Engine review include -- moderation, accept and reject code
 */

/**
 * Menu callback for rejection of PSE submission. Maps $submission to new Project
 * Profile node, saves, then sends out notifications.
 */
function transition_pse_submission_accept($submission_nid) {
  // load submission, bail if not found
  $submission = node_load($submission_nid);
  if ($submission === FALSE) return drupal_not_found();

  // OK, map $submission to new $project node, send out, delete $submission
  // build new Project node, add values @todo move to own function
  global $user;
  $project = new stdClass();
  $project->type = 'project_profile';
  $project->uid = $submission->uid;
  $project->status = 1; //(1 or 0): published or not
  // title & body
  $project->title = $submission->title;
  $project->field_project_summary[0]['value'] = $submission->body;
  // just assign the correct LID to our new Project node
  $project->locations[0]['lid'] = $submission->locations[0]['lid'];
  // project type
  $project->field_project_org_type[0]['value'] = $submission->field_project_org_type[0]['value'];
  // set related initiative as releated and web contact to widget owner
  $project->field_initiative[0]['nid'] = $submission->field_submission_initiative[0]['nid'];
  $project->field_webcontact_user[0]['uid'] = $submission->field_submission_webmaster[0]['uid'];
  // set website
  $project->field_website_home[0]['url'] = $submission->field_website_home[0]['url'];
  // set source as PSE
  $project->field_content_source[0]['value'] = 'Widget-PSE';
  // avoid this sending notifications
  $project->notifications_content_disable = 1;
  // Save this node
  node_save($project);

  // if saved ok, send notifications
  if ($project->nid) {
    // tell submitter about the new accepted project
    $author = user_load($project->uid);
    $params = array(
      'node' => $project,
      'author' => $author,
    );
    // send mail, write to logs
    drupal_mail('transition_pse', 'submitter_project_accepted', $author->mail, language_default(), $params);
    watchdog(
      'transition_pse',
      'PSE -- Accepted submission: @title, @project_link',
      array(
        '@title' => $project->title,
        '@project_link' => url('node/' . $project->nid, array('absolute' => TRUE)),
      )
    );
    // all done here, remove this now-defunct $submission node
    node_delete($submission->nid);
  }
  // horid hack to clear the messages created by the deletion of the node
  $_SESSION['messages'] = array();

    // add done, set nice message about new project page
  drupal_set_message(t('The submission has been added to the Project Directory, thank you!'));
  drupal_set_message(t(
    'The project has been created here: <a href="!link">%name</a>',
    array(
      '%name' => $project->title,
      '!link' => url('node/' . $project->nid, array('absolute' => TRUE))
    )
  ));
  // return to user's moderation page
  drupal_goto('user/' . $user->uid . '/widget-moderation');
}

/**
 * Callback for rejection of PSE submission -- sends rejection email and
 * deletes $submission.
 */
function transition_pse_submission_reject($submission_nid) {
  global $user;
  // load submission, bail if not found
  $submission = node_load($submission_nid);
  if ($submission === FALSE) return drupal_not_found();

  // OK, load author of $submission, send message of rejection
  $author = user_load($submission->uid);
  module_load_include('inc', 'transition_pse', 'transition_pse.pages');
  $params = array(
    'node' => $submission,
    'author' => $author,
  );
  // send mail, make note in log
  drupal_mail('transition_pse', 'submitter_project_rejected', $author->mail, language_default(), $params);
  watchdog(
    'transition_pse',
    'PSE -- Rejected submission: @title, @summary',
    array(
      '@title' => $submission->title,
      '@summary' => $submission->body,
    )
  );
  // delete the submission
  node_delete($submission->nid);
  // horid hack to clear the messages created by the deletion of the node
  $_SESSION['messages'] = array();

  // add done, set nice message and return to user's moderation page
  drupal_set_message(t('Project submission "%name" rejected and deleted, thank you.', array('%name' => $submission->title)));
  drupal_goto('user/' . $user->uid . '/widget-moderation');
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