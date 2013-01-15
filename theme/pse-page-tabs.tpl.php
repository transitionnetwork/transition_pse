<?php
// $Id: pse-page-tabs.tpl.php

/**
 * @file pse-page-tabs.tpl.php
 * Wraps PSE content in head and foot elements.
 *
 * Available variables:
 * - $step: The step (1-3) the page is on.
 */

?>
<div class="inner">
  <div class="tab first<?php if ($step == 1) print ' active'; else print ' inactive';?>">
    <div class="headwrap">
      <div class="g1"><h2>Step 1: About you</h2></div>
        <div class="g2">
        <ol id="progress">
          <li class="details-step current">
              <a href="#">
              <strong><span>About you</span>
               Your name and email.</strong>
              <span class="arrow"></span></a>
          </li>
          <li class="project-step">
              <a href="#">
               <strong><span>Add the project</span>
               Add the details.</strong>
              </a>
              </li>
              <li class="fin-step last">
              <a href="#">
              <span>Finished!</span>
              All done!
              </a>
            </li>
          </ol>
      <img class="logos" alt="Transition Network and Nominet Logos" src="<?php print base_path() . path_to_theme(); ?>/images/tn_nominet_logo_pse.png" />
  </div><!--end g2 -->
  <div class="cf"></div>
  </div><!--end headwrap -->
<div class="helper"><a class="tooltip" href="#">Need Help?<span class="pop"><strong>Why do you need me to register or login?</strong><br />You may wish to update your projects details in the future or we may need to contact you to find out more about this project.</span></a></div>
  </div><!--end step 1 -->
  <div class="tab middle<?php if ($step == 2) print ' active'; else print ' inactive';?>">
       <div class="headwrap">
            <div class="g1"><h2>Step 2: Tell us about the Project</h2></div>
            <div class="g2">
            <ol id="progress">
                <li class="details-step">
                    <a href="#">
                    <span>About you</span>
                     Your name and email.
                    <span class="arrow"></span></a>
                </li>
                <li class="project-step current">
                    <a href="#">
                     <strong><span>Add the project</span>
                     Add the details.</strong>
                    </a>
                    </li>
                    <li class="fin-step last">
                    <a href="#">
                    <span>Finished!</span>
                    All done!
                    </a>
                  </li>
                  </ol>

              <img class="logos" alt="Transition Network and Nominet Logos" src="<?php print base_path() . path_to_theme(); ?>/images/tn_nominet_logo_pse.png" />

          </div><!--end g2 -->
          <div class="cf"></div>
          </div><!--end headwrap -->
        <div class="helper"><a class="tooltip" href="#">Need Help?<span class="pop"><strong>What should I write?</strong><br />This simple form is for you to add details of a project that you may be involved in, or maybe you know of a project locally. Fill in as much detail as you can to describe the project.</span></a></div>
  </div><!--end step 2 -->
  <div class="tab last<?php if ($step == 3) print ' active'; else print ' inactive';?>">
    <div class="headwrap">
            <div class="g1"><h2>Thank you!</h2></div>
            <div class="g2">
            <ol id="progress">
                <li class="details-step">
                    <a href="#">
                    <span>About you</span>
                     Your name and email.
                    <span class="arrow"></span></a>
                </li>
                <li class="project-step">
                    <a href="#">
                     <span>Add the project</span>
                     Add the details.
                    </a>
                    </li>
                    <li class="fin-step last current">
                    <a href="#">
                    <strong><span>Finished!</span>
                    All done!</strong>
                    </a>
                  </li>
                  </ol>

              <img class="logos" alt="Transition Network and Nominet Logos" src="<?php print base_path() . path_to_theme(); ?>/images/tn_nominet_logo_pse.png" />

          </div><!--end g2 -->
          <div class="cf"></div>
          </div><!--end headwrap -->

  </div><!--end step 3 -->
</div>
