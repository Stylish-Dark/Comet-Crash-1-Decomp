# OBJ source-index and vertex-splitting recovery

This note closes the remaining source-index boundary inside model loader
`0x00105308`. The recovered behavior is intentionally narrower than a
general-purpose Wavefront OBJ implementation because the native port needs the
semantics of the shipped NPEB00142 data, not guessed parser features.

## Tokenization and accepted face shapes

The loader tokenizes each OBJ line with the literal delimiter set
`" /\\n\\t"`. Slash is therefore a delimiter and repeated delimiters are
collapsed by the tokenizer. A textual `v//vn` corner contributes two numeric
tokens rather than an explicit empty `vt` token.

The face dispatch at `0x00109A30` accepts exactly these total token counts,
including the leading `f` keyword:

| total tokens | component tokens | accepted source form | emitted corners |
| ---: | ---: | --- | ---: |
| 7 | 6 | triangle `v//vn` | 3 |
| 10 | 9 | triangle `v/vt/vn` | 3 |
| 13 | 12 | quad `v/vt/vn` | 6 |

Other token counts return to the line loop without staging a face.

The 13-token quad path first emits corners 0,1,2, then emits corners 2,3,0.
That is the original triangulation order, not a native-port convenience.

An audit of the exact shipped data contains 16,751 face statements:
5,324 position/normal triangles, 11,412 full triangles, and 15 full quads.
No shipped face falls outside the three recovered forms. Quad expansion raises
the final staged corner/index count to 50,298.

## Source-index conversion

Every present component is parsed in base 10 by the routine called at
`0x00109A9C`, `0x00109AD8`, and corresponding sibling sites, then decremented
exactly once before the 12-byte staging record is appended.

Therefore the conversion is simply:

```text
internal_index = parsed_decimal_index - 1
```

There is no branch implementing Wavefront negative relative-index conversion.
For example, textual `-1` would become internal `-2`, not the final source
element. The shipped corpus uses positive one-based indices only.

The native helper `normalize_legacy_obj_index()` preserves this fact as
reversing provenance. The higher-level native collapse rejects negative or
out-of-range references instead of reproducing the original out-of-bounds
failure mode.

## Internal 12-byte face-reference order

The temporary reference record is:

```text
+0x00  int32 position_index
+0x04  int32 normal_index
+0x08  int32 texcoord_index
stride 0x0C
```

This is not textual OBJ `v/vt/vn` order. Full corners are reordered from
`v,vt,vn` to `position,normal,texcoord` as they are staged. The 7-token
`v//vn` path stores `-1` in the texcoord slot.

The later final-index pass reads the first word after collapse and writes it as
a 16-bit index, which confirms that `+0x00` is repurposed from source position
index to final interleaved-vertex index.

## Collapse is vertex splitting, not global deduplication

The original algorithm begins with one 0x20-byte interleaved vertex per OBJ
`v` position. Normal and UV lanes begin at zero.

For each staged face corner:

1. select the base vertex by the original position index;
2. if the base normal is effectively unclaimed, copy the referenced normal and,
   when present, UV into that base vertex and reuse the base index;
3. otherwise compare the referenced attributes against the base vertex;
4. if the relevant attributes match, reuse the base index;
5. if they differ, append a **fresh** split vertex and rewrite this face
   reference to that new index.

The important legacy quirk is step 5: the loader never searches previously
appended split vertices. Two later corners with the same non-base attribute
combination can therefore create two separate split vertices. The operation is
best described as base-vertex claiming plus per-corner splitting, not a global
unique-tuple deduplication pass.

## Comparison tolerance

The comparison constant loaded at `0x00106138` is the exact single-precision
value represented by `0x38D1B717`, approximately `0.0001f`.

A base normal is treated as unclaimed while all three absolute components are
at most that tolerance. Attribute components are considered mismatched when
their absolute difference is greater than the same tolerance.

For a missing texcoord (`texcoord_index == -1`), only normal comparison is
performed and split vertices receive zero UVs.

For full `v/vt/vn` corners, the shipped asset path compares normal and UV
components. If either relevant attribute differs beyond tolerance, a fresh
split is emitted.

## Native representation

The recovered behavior is represented by:

- `decomp/include/comet/model_obj_semantics.hpp`
- `decomp/src/model_obj_semantics.cpp`

The native representation uses typed vectors and validates source ranges. It
does not preserve the PS3 STL layout, raw pointer arithmetic, or unsafe
negative-index behavior.

The legacy normal-split gate is retained as
`ObjReferenceCollapseOptions::split_on_normal_mismatch` because the binary
derives it from parser state. The exact shipped OBJ/MTL corpus takes the enabled
path; keeping the gate explicit prevents an uncertain provenance detail from
being disguised as a universal OBJ rule.
