<?php

/**
 * @file
 * Log_Timesheet Field API Functions
 */

/**
 * Implements hook_field_info().
 */
function log_timesheet_field_info() {
  return array(
    'log_timesheet' => array(
      'label' => t('Log Timesheet'),
      'description' => t('This field stores a timesheet start, end and total time.'),
      'settings' => array(),
      'instance_settings' => array(),
      'default_widget' => 'log_timesheet_default',
      'default_formatter' => 'log_timesheet_text',
    ),
  );
}

/**
 * Implements hook_field_validate().
 *
 * @see log_timesheet_field_widget_error()
 */
function log_timesheet_field_validate($entity_type, $entity, $field, $instance, $langcode, $items, &$errors) {
  foreach ($items as $delta => $item) {
    $valid_total = (isset($item['total']) || is_numeric($item['total']));
    $valid_start = (!empty($item['start']) && is_numeric($item['start']));
    $valid_end = (!empty($item['end']) && is_numeric($item['end']));

    // Check if both a valid Start and End are provided
    if (!$valid_start xor !$valid_end) {
      $errors[$field['field_name']][$langcode][$delta][] = array(
        'error' => 'log_timesheet_invalid',
        'message' => t('Must provide both Start and End time, or none.'),
      );
    }

    // Check if provided End time is before the Start time
    if ($valid_start && $valid_end) {
      if ($item['end'] < $item['start']) {
        $errors[$field['field_name']][$langcode][$delta][] = array(
          'error' => 'log_timesheet_invalid',
          'message' => t('End time must be greater than or equal to start time.'),
        );
      }
    }

    // Check for invalid Total time
    if (!$valid_total) {
      $errors[$field['field_name']][$langcode][$delta][] = array(
        'error' => 'log_timesheet_invalid',
        'message' => t('Invalid total time.'),
        );
    }

    // Check for negative Total time
    if ($valid_total) {
      if ($item['total'] < 0) {
        $errors[$field['field_name']][$langcode][$delta][] = array(
          'error' => 'log_timesheet_invalid',
          'message' => t('Total time must be positive, or 0.'),
        );
      }
    }


    }
}

/**
 * Implements hook_field_is_empty().
 *
 */
 function log_timesheet_field_is_empty($item, $field) {
   $total = !empty($item['total']);
   $start = !empty($item['start']);
   $end = !empty($item['end']);
   // if no total provided, OR no start & end provided, field is empty
   return !$total && !($start && $end);
 }

 /**
 * Implements hook_field_formatter_info().
 */
 function log_timesheet_field_formatter_info() {
   return array(
     'log_timesheet_text' => array(
       'label' => t('Simple timesheet text'),
       'field types' => array('log_timesheet'),
     ),
     'log_timesheet_date_time_text' => array(
       'label' => t('Log Timesheet Date/Time'),
       'field types' => array('log_timesheet'),
     ),
   );
 }

/**
 * Implements hook_field_formatter_view().
 */
function log_timesheet_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  $element = array();

  foreach ($items as $delta => $item) {
    $output = '';

    switch ($display['type']) {
      case 'log_timesheet_text':
        $output = t(
          'Start: @start, End: @end, Total: @total',
          array(
            '@start' => $item['start'],
            '@end' => $item['end'],
            '@total' => $item['total']
          )
        );
        break;

      case 'log_timesheet_date_time_text':
        $output = t(
          'Start: @start<br>End: @end<br>Total: @total',
          array(
            '@start' => format_date($item['start'], 'html_datetime'),
            '@end' => format_date($item['end'], 'html_datetime'),
            '@total' => format_interval($item['total'], 6),
          )
        );
    }

    // Create the markup element.
    $element[$delta] = array(
      '#type' => 'markup',
      '#markup' => $output,
    );
  }


  return $element;
}

/**
 * Implements hook_field_widget_info().
 */
function log_timesheet_field_widget_info() {
  return array(
    'log_timesheet_default' => array(
      'label' => t('Log Timesheet'),
      'field types' => array('log_timesheet'),
      'settings' => array(),
      'behaviors' => array(
        'multiple values' => FIELD_BEHAVIOR_DEFAULT,
        'default value' => FIELD_BEHAVIOR_DEFAULT,
      ),
    ),
  );
}

/**
 * Implements hook_field_widget_form().
 */
function log_timesheet_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
    switch ($instance['widget']['type']) {

    case 'log_timesheet_default':
      $start = isset($items[$delta]['start']) ? $items[$delta]['start'] : '0';
      $end = isset($items[$delta]['end']) ? $items[$delta]['end'] : '0';
      $total = isset($items[$delta]['total']) ? $items[$delta]['total'] : '0';

      // Make a fieldset with three text fields
      $element['#type'] = 'fieldset';
      $element['#element_validate'] = array('log_timesheet_default_widget_validate');
      $element['start'] = array(
        '#type' => 'textfield',
        '#title' => t('Start'),
        '#default_value' => $start,
        '#description' => t('The start time of the timesheet'),
        '#element_validate' => array('element_validate_integer'),
        '#size' => 15,
      );

      $element['end'] = array(
        '#type' => 'textfield',
        '#title' => t('End'),
        '#default_value' => $end,
        '#description' => t('The end time of the timesheet'),
        '#element_validate' => array('element_validate_integer'),
        '#size' => 15,
      );

      $element['total'] = array(
        '#type' => 'textfield',
        '#title' => t('Total'),
        '#default_value' => $total,
        '#description' => t('The total time of the timesheet'),
        '#element_validate' => array('element_validate_integer'),
        '#size' => 15,
      );
      break;
  }

  return $element;
}

/**
 * Validate the Total time field.
 *
 * Calculate the Total time from the provided Start and End time
 * or modify the End time if a Total time is provided.
 *
 */
function log_timesheet_default_widget_validate($element, &$form_state) {
  // Search through the form values to find the current field value.
  $parents = $element['#parents'];
  $values = drupal_array_get_nested_value($form_state['values'], $parents);

  // Check if Start and End time are provided
  if (!empty($values['start']) && is_numeric($values['start']) && !empty($values['end']) && is_numeric($values['start'])) {
    // Calculate total time based on provided Start and End time
    $calculated_total = $values['end'] - $values['start'];

    // Update the Total time if not provided
    if (empty($values['total']) || !is_numeric($values['total'])) {
      $values['total'] = $calculated_total;
    } else {

      // Check if provided Total time matches provided Start and End time
      if ($values['total'] != $calculated_total) {
        // Update End time to match provided Total time
        $values['end'] = $values['start'] + $values['total'];
      }
    }
  }

  // Provide fake Log ID and User ID for now
  $values['id'] = 0;
  $values['uid'] = 0;

  form_set_value($element, $values, $form_state);
}

/**
 * Implements hook_field_widget_error().
 */
function log_timesheet_field_widget_error($element, $error, $form, &$form_state) {
  switch ($error['error']) {
    case 'log_timesheet_invalid':
      form_error($element, $error['message']);
      break;
  }
}
