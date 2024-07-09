Test Setup
==========

Software / Hardware setup
-------------------------

.. list-table::
   :widths: 25 75
   :header-rows: 1

   * - Item
     - Description

{% for item, description in sysinfo.items() %}
   * - **{{item}}**
  {% if description is mapping %}
     - 
       .. class:: tablebullet
       {% for key, value in description.items() %}
       * {{key}}: {{value}}
       {% endfor %}
  {% elif description is iterable and (description is not string) %}
     - 
       .. class:: tablebullet
       {% for value in description %}
       * {{value}}
      {% endfor %}   
  {% else %}
     - {{description}}
  {% endif %}
{% endfor %}

BIOS
----

.. list-table:: 
   :widths: 25 75
   :header-rows: 1

   * - Item
     - Description
   * - BIOS name
     - {{biosinfo.bios}}
   * - BIOS version
     - {{biosinfo.bios_version}}