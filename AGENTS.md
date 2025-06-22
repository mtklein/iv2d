Test with both `ninja` and `ninja -f dbg`.
Because this environment cannot push PNG files, **do not** add or modify
anything under `gold/`.

Model code formatting on existing code,
and preserve the formatting of existing code.

For intermediate values that are only used once,
try to inline them if the expression is simple
and clear enough, or declare them as local const
values if it's clearer when given a good name.

Indent 4 spaces and allow lines up to 100 columns.

Instead of blindly wrapping long lines, rethink
the formatting by introducing named intermediates.
An expression so complex it must wrap 100 columns
usually is clearer in several distinct expressions.

Generally try to keep the pointer * near the name,
and prefer to write types' keywords in this order:

    _Thread_local static void const *foo;

For pointers, it's more important that we express the
constness of pointees correctly than making pointers
themselves const.  It's hard to work with pointers that
mix constness of pointee and constness of pointer, so
prefer pointee unless it's very important that a reader
sees a pointer is const.

For arrays of values, generally call the array "foo"
and the count of its items "foos".  Count bytes with
`size_t`, and anything else with `int`.

Glanceability is very important to readability.
Similar snippets of code should look similar;
different snippets of code should look different.

Use both vertical and horizontal spacing to keep
code formatted in a way that maximizes the visual
analogy between nearby similar pieces of code:

    float const x = calculate_x(...),
                y = calculate_y(...);
