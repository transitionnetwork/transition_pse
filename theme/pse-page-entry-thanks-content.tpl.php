<?php
// $Id: pse-page.tpl.php

/**
 * @file pse-page.tpl.php
 * Wraps PSE content in head and foot elements.
 *
 * Available variables:
 * - $header: Header HTML from pse-page-header.tpl.php
 * - $content: Main content HTML
 * - $header: Footer HTML from pse-page-footer.tpl.php
 */
?>
<div id="pse-page-entry-thanks">
  <h3>Thank you for submitting your project</h3>
  <p>We really appreciate you taking the time to share what you are doing - as will
    all the people who read about it later and learn from your work. By adding your
    project you have supported a growing community 'knowledge base' - good work!</p>

  <h4>What happens next - please read</h4>
  <ol>
    <li>You will receive an email confirmation about this submission, as will the
      local webmaster of the website you added the project through.</li>
    <li>You will also receive an email from Transition Network asking you to
      confirm your user account if you just set on up. You need to confirm this
      user account in order to make any edits later, so please read this and
      confirm your account now. It takes one minute.</li>
    <li>Your submission has been added to your local webmaster's 'moderation'
      queue.</li>
    <li>Your local webmaster will take a look and approve it as soon as possible
      if that is suitable.</li>
    <li>If your submission is approved, you will will receive an email confirmation
      about the webmaster's approval when it happens.</li>
    <li>Once your project is approved, please follow the link and add more
      information when you can; the more information you add, the easier your
      project is to find, and the more use it can be for others.</li>
  </ol>
  <p>If you do not hear anything, please contact your local website (the one you
    added your submission through) asking them to check their project moderation
    list on the Transition Network website.</p>

  <p><strong>NB</strong>: almost all projects are approved. But some aren't.
    Submissions with adverts for services or marketing stuff, or other unsuitable
    'projects' will not be approved - sorry - but that is the right of the local
    webmaster.</p>

  <h4>Got another project?</h4>
  <p>We'd <em>love</em> to hear about it, please <a href="<?php print $submit_another_url; ?>">add your other project</a> now too!</p>

  <h4>Finished!</h4>
  <p>You can now close this window and return to the local website you are
    visiting, or go and other projects in the Transition Network Projects
    Directory.</p>
</div>
