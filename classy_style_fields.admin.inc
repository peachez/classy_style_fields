<?php

/**
 * Implements hook_form_FORM_ID_alter()
 */
function classy_style_fields_form_classy_paragraphs_style_add_form_alter(&$form, &$form_state, $form_id) {
  _classy_style_fields_add_config_fields($form, $form_state, $form_id);
}

/**
 * Implements hook_form_FORM_ID_alter()
 */
function classy_style_fields_form_classy_paragraphs_style_edit_form_alter(&$form, &$form_state, $form_id) {
  _classy_style_fields_add_config_fields($form, $form_state, $form_id);
}

/**
 * Common form alter function to the ADD and EDIT forms for classy paragraph styles.
 */
function _classy_style_fields_add_config_fields(&$form, &$form_state, $form_id) {
  $all_classy_fields = _classy_style_fields_get_all_classy_paragraph_fields();
  $entity_and_bundle_names = _classy_style_fields_get_entity_and_bundle_names();

  // Add admin css for checkbox style
  // kint($element);
  $form['#attached']['library'][] = 'classy_style_fields/classy_style_admin';

  $form['classy_fields'] = [
    '#type' => 'fieldgroup',
    '#title' => t('Classy Fields'),
    '#tree' => TRUE,
  ];
  $form['classy_fields']['desc'] = [
    '#markup' => t('All classy paragraph style fields currently configured are shown here. Each classy paragraph reference field is listed by Entity Type, then by the bundle in which it is configured.') . '<br>' . t('Select the appropriate fields that should have this style applied. Keep in mind, the css may still need to be written for any selection made. Try to keep that css as general as possible.') . '<br>' . t('Any fields left unselected on <u>ALL</u> classy styles will automatically have this style, and all other styles, available.'),
  ];

  foreach ($all_classy_fields as $entity_type => $bundles_info) {
    $entity_type_label = $entity_and_bundle_names[$entity_type]['label'];
    $options = [];

    $form['classy_fields'][$entity_type] = [
      '#type' => 'fieldgroup',
      '#title' => t($entity_type_label),
    ];

    foreach ($bundles_info as $bundle_name => $fields) {
      // Build the field checkboxes options
      $options = [];
      foreach ($fields as $field) {
        $field_label = $entity_and_bundle_names[$entity_type]['bundles'][$bundle_name]['fields'][$field];
        $key = _classy_style_fields_generate_option_name($entity_type, $bundle_name, $field);
        $options[$key] = $field_label;
      }

      $classy_name = $form['id']['#default_value']; // "2_column_40_60"
      $selected_options = _classy_style_fields_get_classy_saved_options($form_id, $classy_name, $entity_type, $bundle_name);

      $bundle_label = $entity_and_bundle_names[$entity_type]['bundles'][$bundle_name]['label'];
      $form['classy_fields'][$entity_type][$bundle_name] = [
        '#title' => t($bundle_label),
        '#type' => 'checkboxes',
        '#options' => $options,
        '#default_value' => $selected_options,
      ];
    }
  }

  $form['actions']['submit']['#submit'][] = 'classy_style_fields_add_config_fields_submit';
}

/**
 * Custom callback to modify entity types, bundles, and fields config settings
 * when a classy paragraph style is saved.
 */
function classy_style_fields_add_config_fields_submit($form, &$form_state) {
  // Get the current saved options
  $entity_fields = \Drupal::configFactory()->get('classy_style_fields.settings')->get(CLASSY_STYLE_FIELDS_CLASSY_FIELDS);
  $entity_fields = empty($entity_fields) ? [] : $entity_fields;

  $values = $form_state->getValues();
  $classy_style = $values['id'];
  foreach ($values['classy_fields'] as $entity_type => $bundle_info) {
    foreach ($bundle_info as $bundle_name => $ebf_info) {
      // for easy ref
      $eb_fields = &$entity_fields[$classy_style][$entity_type][$bundle_name];
      $eb_fields = empty($eb_fields) ? [] : $eb_fields;
      foreach ($ebf_info as $ebf_name => $selection_value) {
        [$e, $b, $field_name] = explode('__', $ebf_name);
        $field_position = array_search($field_name, $eb_fields);
        $field_in_config = is_numeric($field_position);

        // Save the selected field for this style
        if (!empty($selection_value) && !$field_in_config) {
          $eb_fields[] = $field_name;
        }
        // Remove the de-selected frield from this style
        elseif (empty($selection_value) && $field_in_config) {
          unset($eb_fields[$field_position]);
        }
      }
      $eb_fields = array_values($eb_fields);
    }
  }

  \Drupal::configFactory()->getEditable('classy_style_fields.settings')
    ->set(CLASSY_STYLE_FIELDS_CLASSY_FIELDS, $entity_fields)
    ->save();
}

/**
 * Builds a sdata structur of all classy paragraph fields nested by entity type and bundles.
 *
 * Example Structure:
 * [
 *   'entity_type' => [
 *     'bundle' => [
 *       'field_name',
 *        ...
 *     ],
 *     'bundle2' => [
 *       'field_name',
 *        ...
 *     ],
 *   ],
 *   'entity_type_2' => [
 *     'another_bundle' => [
 *       'field_name',
 *        ...
 *     ],
 *   ],
 * ]
 */
function _classy_style_fields_get_all_classy_paragraph_fields() {
  $ef_manager = \Drupal::service('entity_field.manager');

  $my_fields = [];
  foreach ($ef_manager->getFieldMap() as $entity_type => $fields_list) {
    foreach ($fields_list as $field_name => $info) {
      if (strpos($field_name, 'field_') === 0 && $info['type'] == 'entity_reference' && !empty($info['bundles'])) {
        foreach ($info['bundles'] as $bundle) {
          $definitions = \Drupal::service('entity_field.manager')->getFieldDefinitions($entity_type, $bundle);

          $settings = $definitions[$field_name]->getSettings();
          if (isset($settings['target_type']) && $settings['target_type'] == 'classy_paragraphs_style') {
            // Found one, add it in.
            $my_fields[$entity_type][$bundle][] = $field_name;
          }
        }
      }
    }
  }

  return $my_fields;
}

/**
 * Builds a list of entity type, bundle, and field name labels.
 *
 * Example structure:
 *   [
 *     'entity_type' => [
 *       'label' => 'Entity Type Label',
 *       'bundles' => [
 *         'bundle_name' => [
 *           'label' => 'Bundle Label',
 *           'fields' => [
 *             'field_name' => 'Field Label',
 *             ...
 *           ]
 *         ],
 *       ]
 *     ]
 *   ]
 */
function _classy_style_fields_get_entity_and_bundle_names() {
  $bundle_names = [];
  $bundles_info = \Drupal::service("entity_type.bundle.info")->getAllBundleInfo();
  foreach ($bundles_info as $entity_type => $bundle_info) {
    $bundle_names[$entity_type] = ['label' => ucwords(str_replace('_', ' ', $entity_type)), 'bundles' => []];
    foreach ($bundle_info as $bundle => $info) {
      // Entity types with this are entities representing the different types, not actual entities we need.
      if ($info['label'] instanceof Drupal\Core\StringTranslation\TranslatableMarkup) { continue; }

      $bundle_names[$entity_type]['bundles'][$bundle] = [
        'label' => $info['label'],
        'fields' => [],
      ];

      $definitions = \Drupal::service('entity_field.manager')->getFieldDefinitions($entity_type, $bundle);
      foreach ($definitions as $field_name => $def) {
        // We only limit to actual fields.
        // If we need more later on, just remove this check.
        if (strpos($field_name, 'field_') !== 0) { continue; }

        $bundle_names[$entity_type]['bundles'][$bundle]['fields'][$field_name] = $def->getLabel();
      }
    }
  }

  // Dont keep any entotye type with an empty bundles set.
  $bundle_names = array_filter($bundle_names, function($bundle_name) {
    return !empty($bundle_name['bundles']);
  });
  ksort($bundle_names);
  return $bundle_names;
}

/**
 * Convert selected fields into the appropriate array of keys names for the checkboxes selections.
 *
 * Eample data structure of config settings
 *   classy_style_fields.settings.CLASSY_STYLE_FIELDS_CLASSY_FIELDS
 *   [
 *     'classy_name' => [
 *       'entity_type' => [
 *         'bundle' => [
 *           'field_name',
 *           ...
 *         ],
 *       ],
 *     ],
 *   ]
 */
function _classy_style_fields_get_classy_saved_options($form_id, $classy_name, $entity_type, $bundle_name) {
  $classy_selection = [];
  if ($form_id == 'classy_paragraphs_style_edit_form') {
    $entity_fields = \Drupal::configFactory()->get('classy_style_fields.settings')->get(CLASSY_STYLE_FIELDS_CLASSY_FIELDS);
    $entity_fields = empty($entity_fields) ? [] : $entity_fields;

    if (isset($entity_fields[$classy_name][$entity_type][$bundle_name])) {
      $field_names = $entity_fields[$classy_name][$entity_type][$bundle_name]; // for easy ref
      $classy_selection = array_map(function($field_name) use ($entity_type, $bundle_name) {
        return _classy_style_fields_generate_option_name($entity_type, $bundle_name, $field_name);
      }, $field_names);
    }
  }

  return $classy_selection;
}

/**
 * Helper function to build a key name for a field of a bundle on an entity type.
 */
function _classy_style_fields_generate_option_name($entity_type, $bundle_name, $field_name) {
  return implode('__', [$entity_type, $bundle_name, $field_name]);
}