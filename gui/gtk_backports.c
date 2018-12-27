
#if !GTK_CHECK_VERSION(3,16,0)

/* gtk_text_view_set_monospace() requres 3.16 */

void gtk_text_view_set_monospace (GtkTextView *text_view, gboolean monospace)
{ /*no-op*/ }

/* gtk_text_buffer_insert_markup() requires 3.16 */

static GtkTextTag *
get_tag_for_attributes (PangoAttrIterator *iter)
{
  PangoAttribute *attr;
  GtkTextTag *tag;

  tag = gtk_text_tag_new (NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_LANGUAGE);
  if (attr)
    g_object_set (tag, "language", pango_language_to_string (((PangoAttrLanguage*)attr)->value), NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_FAMILY);
  if (attr)
    g_object_set (tag, "family", ((PangoAttrString*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_STYLE);
  if (attr)
    g_object_set (tag, "style", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_WEIGHT);
  if (attr)
    g_object_set (tag, "weight", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_VARIANT);
  if (attr)
    g_object_set (tag, "variant", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_STRETCH);
  if (attr)
    g_object_set (tag, "stretch", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_SIZE);
  if (attr)
    g_object_set (tag, "size", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_FONT_DESC);
  if (attr)
    g_object_set (tag, "font-desc", ((PangoAttrFontDesc*)attr)->desc, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_FOREGROUND);
  if (attr)
    {
      PangoColor *color;
      GdkRGBA rgba;

      color = &((PangoAttrColor*)attr)->color;
      rgba.red = color->red / 65535.;
      rgba.green = color->green / 65535.;
      rgba.blue = color->blue / 65535.;
      rgba.alpha = 1.;
      g_object_set (tag, "foreground-rgba", &rgba, NULL);
    };

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_BACKGROUND);
  if (attr)
    {
      PangoColor *color;
      GdkRGBA rgba;

      color = &((PangoAttrColor*)attr)->color;
      rgba.red = color->red / 65535.;
      rgba.green = color->green / 65535.;
      rgba.blue = color->blue / 65535.;
      rgba.alpha = 1.;
      g_object_set (tag, "background-rgba", &rgba, NULL);
    };

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_UNDERLINE);
  if (attr)
    g_object_set (tag, "underline", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_UNDERLINE_COLOR);
  if (attr)
    {
      PangoColor *color;
      GdkRGBA rgba;

      color = &((PangoAttrColor*)attr)->color;
      rgba.red = color->red / 65535.;
      rgba.green = color->green / 65535.;
      rgba.blue = color->blue / 65535.;
      rgba.alpha = 1.;
      g_object_set (tag, "underline-rgba", &rgba, NULL);
    }

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_STRIKETHROUGH);
  if (attr)
    g_object_set (tag, "strikethrough", (gboolean) (((PangoAttrInt*)attr)->value != 0), NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_STRIKETHROUGH_COLOR);
  if (attr)
    {
      PangoColor *color;
      GdkRGBA rgba;

      color = &((PangoAttrColor*)attr)->color;
      rgba.red = color->red / 65535.;
      rgba.green = color->green / 65535.;
      rgba.blue = color->blue / 65535.;
      rgba.alpha = 1.;
      g_object_set (tag, "strikethrough-rgba", &rgba, NULL);
    }

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_RISE);
  if (attr)
    g_object_set (tag, "rise", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_SCALE);
  if (attr)
    g_object_set (tag, "scale", ((PangoAttrFloat*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_FALLBACK);
  if (attr)
    g_object_set (tag, "fallback", (gboolean) (((PangoAttrInt*)attr)->value != 0), NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_LETTER_SPACING);
  if (attr)
    g_object_set (tag, "letter-spacing", ((PangoAttrInt*)attr)->value, NULL);

#if PANGO_VERSION_CHECK(1,38,0)
  attr = pango_attr_iterator_get (iter, PANGO_ATTR_FONT_FEATURES);
  if (attr)
    g_object_set (tag, "font-features", ((PangoAttrString*)attr)->value, NULL);
#endif

  return tag;
}

static void
gtk_text_buffer_insert_with_attributes (GtkTextBuffer *buffer,
                                        GtkTextIter   *iter,
                                        const gchar   *text,
                                        PangoAttrList *attributes)
{
  GtkTextMark *mark;
  PangoAttrIterator *attr;
  GtkTextTagTable *tags;

  g_return_if_fail (GTK_IS_TEXT_BUFFER (buffer));

  if (!attributes)
    {
      gtk_text_buffer_insert (buffer, iter, text, -1);
      return;
    }

  /* create mark with right gravity */
  mark = gtk_text_buffer_create_mark (buffer, NULL, iter, FALSE);
  attr = pango_attr_list_get_iterator (attributes);
  tags = gtk_text_buffer_get_tag_table (buffer);

  do
    {
      GtkTextTag *tag;
      gint start, end;

      pango_attr_iterator_range (attr, &start, &end);

      if (end == G_MAXINT) /* last chunk */
        end = start - 1; /* resulting in -1 to be passed to _insert */

      tag = get_tag_for_attributes (attr);
      gtk_text_tag_table_add (tags, tag);

      gtk_text_buffer_insert_with_tags (buffer, iter, text + start, end - start, tag, NULL);

      gtk_text_buffer_get_iter_at_mark (buffer, iter, mark);
    }
  while (pango_attr_iterator_next (attr));

  gtk_text_buffer_delete_mark (buffer, mark);
  pango_attr_iterator_destroy (attr);
}


/**
 * gtk_text_buffer_insert_markup:
 * @buffer: a #GtkTextBuffer
 * @iter: location to insert the markup
 * @markup: a nul-terminated UTF-8 string containing [Pango markup][PangoMarkupFormat]
 * @len: length of @markup in bytes, or -1
 *
 * Inserts the text in @markup at position @iter. @markup will be inserted
 * in its entirety and must be nul-terminated and valid UTF-8. Emits the
 * #GtkTextBuffer::insert-text signal, possibly multiple times; insertion
 * actually occurs in the default handler for the signal. @iter will point
 * to the end of the inserted text on return.
 */
void
gtk_text_buffer_insert_markup (GtkTextBuffer *buffer,
                               GtkTextIter   *iter,
                               const gchar   *markup,
                               gint           len)
{
  PangoAttrList *attributes;
  gchar *text;
  GError *error = NULL;

  if (!pango_parse_markup (markup, len, 0, &attributes, &text, NULL, &error))
    {
      g_warning ("Invalid markup string: %s", error->message);
      g_error_free (error);
      return;
    }

  gtk_text_buffer_insert_with_attributes (buffer, iter, text, attributes);

  pango_attr_list_unref (attributes);
  g_free (text);
}
#endif
