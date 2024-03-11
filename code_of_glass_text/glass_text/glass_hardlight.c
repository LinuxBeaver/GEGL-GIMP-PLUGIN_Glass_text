/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 * Beaver 2023, Glass Text with Hard Light

1. DO NOT CONFUSE THIS PLUGIN WITH "GLASS OVER TEXT" another plugin of mine. 

2. This plugin requires two other plugins to work (glossy balloon and metallic) don't worry, it ships with them.

3. Users can test this filter without installing by pasting this syntax into Gimp's GEGL Graph filter.

4. Fun fact, This is the first plugin of mine other then "Graphical Effects experimental to use glossy balloon as a dependency. 

5. Fun Fact 2, when this filter was being developed it required the hard light blend mode.


median-blur radius=7 alpha-percentile=3
lb:glossy-balloon gaus=1 crop median-blur radius=0 gaussian-blur std-dev-x=1.5 std-dev-y=1.5
metallic  
liquid=1.0900000000000001 
  solar1=2.7000000000000002 
solar2=4.0679999999999996 
 solar3=0.159 
 light=5
 smooth=2 
blend=graph4 
opacity value=4 median-blur radius=0

id=1 gimp:layer-mode layer-mode=replace opacity=1.00 aux=[ ref=1 color-to-alpha color=#9d9d9d   ]

median-blur radius=0

--end of syntax--

 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (size, _("Reverse Size control"), 3)
   description (_("Glass radius. Lower values make it larger and higher values make the glass smaller."))
   value_range (2, 10)
   ui_range    (2, 10)


property_double (glass1, _("Limited Glass control 1"), 2.5)
   description (_("A small portion of alien map's red channel solarization"))
   value_range (2.5, 2.9)
   ui_range    (2.5, 2.9)
  ui_steps      (0.1, 0.50)

property_double (glass2, _("Limited Glass control 2"), 4.1)
   description (_("A small portion of alien map's green channel solarization"))
   value_range (3.8, 4.2)
   ui_range    (3.8, 4.2)
  ui_steps      (0.1, 0.50)

property_double (glass3, _("Limited Glass control 3"), 0.2)
   description (_("A small portion of alien map's blue channel solarization"))
   value_range (0.1, 0.2)
   ui_range    (0.1, 0.2)
  ui_steps      (0.1, 0.50)

property_double (hardlightmode, _("0% for Hard light blend mode"), 0.8)
   description (_("This mode makes a better 'glass text' effect, but requires the user to use Gimp only blend modes 'hard light or linear light' with a image layer below it. To use this mode correctly set the slider to 0, put a another image file on a Gimp layer below and blend the text with hard light. If the image is overtly bright this will not work. This slider can also be used to give the glass some opacity. "))
   value_range (0.0, 1.00)
   ui_range    (0.0, 1.0)
  ui_steps      (0.1, 0.50)


#else

#define GEGL_OP_META
#define GEGL_OP_NAME     glass_hardlight
#define GEGL_OP_C_SOURCE glass_hardlight.c

#include "gegl-op.h"

static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglNode *input, *mediansize, *gbf, *metallic, *idref, *othergraph, *c2a, *replaceblendmode, *output;

  input    = gegl_node_get_input_proxy (gegl, "input");
  output   = gegl_node_get_output_proxy (gegl, "output");

  mediansize = gegl_node_new_child (gegl,
                                  "operation", "gegl:median-blur", "alpha-percentile", 3.0, "neighborhood", 0, "abyss-policy",     GEGL_ABYSS_NONE,
                                  NULL);
#define glossyballoonandfriends \
" lb:glossy-balloon gaus=1  median-blur  abyss-policy=none radius=0 gaussian-blur  abyss-policy=none  clip-extent=false std-dev-x=1.5 std-dev-y=1.5 "\

  gbf = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string", glossyballoonandfriends,
                                  NULL);

  metallic = gegl_node_new_child (gegl,
                                  "operation", "lb:metallic", "liquid", 1.0, "light", 5.0, "smooth", 2, "blend", 3, 
                                  NULL);

/*This is Gimp's replace blend mode. If Gimp ever updates with new blend modes this number will very likely need to be changed to 61 or 63*/
replaceblendmode = gegl_node_new_child (gegl,
                                    "operation", "gimp:layer-mode", "layer-mode", 62, "composite-mode", 0,  "blend-space", 0, NULL);

#define c2ameme \
" color-to-alpha color=#9d9d9d median-blur abyss-policy=none radius=0 "\

  c2a = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string", c2ameme,
                                  NULL);

#define other \
" opacity value=4 median-blur abyss-policy=none radius=0 "\

                               
  othergraph = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string", other,
                                  NULL);

  idref = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);


  gegl_node_link_many (input, mediansize, gbf, metallic, othergraph, idref, replaceblendmode, output, NULL);
  gegl_node_connect (replaceblendmode, "aux", c2a, "output");
  gegl_node_link_many (idref, c2a, NULL);

 gegl_operation_meta_redirect (operation, "size", mediansize, "radius"); 
 gegl_operation_meta_redirect (operation, "glass1", metallic, "solar1"); 
 gegl_operation_meta_redirect (operation, "glass2", metallic, "solar2"); 
 gegl_operation_meta_redirect (operation, "glass3", metallic, "solar3"); 
 gegl_operation_meta_redirect (operation, "hardlightmode", replaceblendmode, "opacity"); 

/*hardlight mode works by setting the replace blend mode to 0 opacity, thus removing the "color to alpha" Thus making a "glass text thing" that requires blending with hard light.
Using Color to alpha's " opacity-threshold" to do the same thing works but creates a never before seen bug that even median blur at zero radius can't solve.*/
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "lb:glasstext",
    "title",       _("Glass Text"),
    "reference-hash", "nfakgjgusehardddlight3453bruh",
    "description", _("Turn normal text into Glass Text. Using the 'hard light' mode slider you can make it where the filter requires Gimp's 'hard light blend mode; which has better result."),
    "gimp:menu-path", "<Image>/Filters/Text Styling",
    "gimp:menu-label", _("Glass Text..."),
    NULL);
}

#endif
