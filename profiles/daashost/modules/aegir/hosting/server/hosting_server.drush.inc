<?php
/**
 * @file
 * :Drush hooks for the Hosting server module.
 */

/**
 * Pass options for the server verification to the backend.
 *
 * Here we pass the hostname, IP addresses, and setup the services for
 * this server (?).
 */
function hosting_hosting_server_context_options(&$task) {
  $task->context_options['remote_host'] = $task->ref->title;
  $ip_list = $task->ref->ip_addresses;
  $task->context_options['ip_addresses'] = sizeof($ip_list) ? implode(',', $ip_list) : 'null';

  foreach (hosting_server_services() as $type => $info) {
    if (isset($task->ref->services[$type])) {
      $service = $task->ref->services[$type];
      if ($service->available) {
        $service->context_options($task->task_type, $task->ref->type, $task);
      }
    }
    else {
      $task->context_options["{$type}_service_type"] = '0';
    }
  }
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function hosting_server_drush_context_import($context, &$node) {
  if ($context->type == 'server') {
    if (!isset($node->title)) {
      $node->title = strtolower($context->remote_host);
    }
    $node->new_ip_addresses = implode("\n", $context->ip_addresses);
    foreach (hosting_server_services() as $type => $info) {
      if (isset($context->{$type . '_service_type'})) {
        $service_type = $context->{$type . '_service_type'};
        if (isset($node->services[$type])) {
          if ($node->services[$type]->type != $service_type) {
            unset($node->services[$type]);
          }
        }
        if (!isset($node->services[$type])) {
          // only support the base configuration things for now.
          hosting_services_add($node, $type, $service_type, $values);
        }

        $node->services[$type]->context_import($context);
      }
    }
  }
}

/**
 * Resave the target node to trigger reverifications.
 */
function hosting_server_post_hosting_verify_task($task, $data) {
  if ($task->ref->type == 'server') {
    $task->ref->no_verify = TRUE;
    $task->ref->verified = REQUEST_TIME; // set verified flag on server, to let it know it has been checked.
    $task->ref->server_status = HOSTING_SERVER_ENABLED;
    node_save($task->ref);

    // Verify the platforms on this server
    $query = db_query("SELECT nid FROM {hosting_platform} WHERE status <> :status AND web_server = :web_server", array(':status' => HOSTING_PLATFORM_DELETED, ':web_server' => $task->ref->nid));
    while ($nid = $query->fetch()) {
      hosting_add_task($nid->nid, 'verify');
    }
  }
}

/**
 * Implements hook_hosting_post_DELETE().
 */
function hosting_server_post_hosting_delete_task($task, $data) {
  $task->ref->server_status = HOSTING_SERVER_DELETED;
  $task->ref->no_verify = TRUE;
  node_save($task->ref);
  hosting_context_delete($task->ref->nid);
}
