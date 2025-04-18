#include "jsi.h"

#ifndef JS_HEAPSORT
#define JS_HEAPSORT 0
#endif

int js_getlength(js_State *J, int idx)
{
	int len;
	js_getproperty(J, idx, "length");
	len = js_tointeger(J, -1);
	js_pop(J, 1);
	return len;
}

void js_setlength(js_State *J, int idx, int len)
{
	js_pushnumber(J, len);
	js_setproperty(J, idx < 0 ? idx - 1 : idx, "length");
}

static void jsB_new_Array(js_State *J)
{
	int i, top = js_gettop(J);

	js_newarray(J);

	if (top == 2) {
		if (js_isnumber(J, 1)) {
			js_copy(J, 1);
			js_setproperty(J, -2, "length");
		} else {
			js_copy(J, 1);
			js_setindex(J, -2, 0);
		}
	} else {
		for (i = 1; i < top; ++i) {
			js_copy(J, i);
			js_setindex(J, -2, i - 1);
		}
	}
}

static void Ap_concat(js_State *J)
{
	int i, top = js_gettop(J);
	int n, k, len;

	js_newarray(J);
	n = 0;

	for (i = 0; i < top; ++i) {
		js_copy(J, i);
		if (js_isarray(J, -1)) {
			len = js_getlength(J, -1);
			for (k = 0; k < len; ++k)
				if (js_hasindex(J, -1, k))
					js_setindex(J, -3, n++);
			js_pop(J, 1);
		} else {
			js_setindex(J, -2, n++);
		}
	}
}

static void Ap_join(js_State *J)
{
	char * volatile out = NULL;
	const char * volatile r = NULL;
	const char *sep;
	int seplen;
	int k, n, len, rlen;

	len = js_getlength(J, 0);

	if (js_isdefined(J, 1)) {
		sep = js_tostring(J, 1);
		seplen = strlen(sep);
	} else {
		sep = ",";
		seplen = 1;
	}

	if (len <= 0) {
		js_pushliteral(J, "");
		return;
	}

	if (js_try(J)) {
		js_free(J, out);
		js_throw(J);
	}

	n = 0;
	for (k = 0; k < len; ++k) {
		js_getindex(J, 0, k);
		if (js_iscoercible(J, -1)) {
			r = js_tostring(J, -1);
			rlen = strlen(r);
		} else {
			rlen = 0;
		}

		if (k == 0) {
			out = js_malloc(J, rlen + 1);
			if (rlen > 0) {
				memcpy(out, r, rlen);
				n += rlen;
			}
		} else {
			if (n + seplen + rlen > JS_STRLIMIT)
				js_rangeerror(J, "invalid string length");
			out = js_realloc(J, out, n + seplen + rlen + 1);
			if (seplen > 0) {
				memcpy(out + n, sep, seplen);
				n += seplen;
			}
			if (rlen > 0) {
				memcpy(out + n, r, rlen);
				n += rlen;
			}
		}

		js_pop(J, 1);
	}

	js_pushlstring(J, out, n);
	js_endtry(J);
	js_free(J, out);
}

static void Ap_pop(js_State *J)
{
	int n;

	n = js_getlength(J, 0);

	if (n > 0) {
		js_getindex(J, 0, n - 1);
		js_delindex(J, 0, n - 1);
		js_setlength(J, 0, n - 1);
	} else {
		js_setlength(J, 0, 0);
		js_pushundefined(J);
	}
}

static void Ap_push(js_State *J)
{
	int i, top = js_gettop(J);
	int n;

	n = js_getlength(J, 0);

	for (i = 1; i < top; ++i, ++n) {
		js_copy(J, i);
		js_setindex(J, 0, n);
	}

	js_setlength(J, 0, n);

	js_pushnumber(J, n);
}

static void Ap_reverse(js_State *J)
{
	int len, middle, lower;

	len = js_getlength(J, 0);
	middle = len / 2;
	lower = 0;

	while (lower != middle) {
		int upper = len - lower - 1;
		int haslower = js_hasindex(J, 0, lower);
		int hasupper = js_hasindex(J, 0, upper);
		if (haslower && hasupper) {
			js_setindex(J, 0, lower);
			js_setindex(J, 0, upper);
		} else if (hasupper) {
			js_setindex(J, 0, lower);
			js_delindex(J, 0, upper);
		} else if (haslower) {
			js_setindex(J, 0, upper);
			js_delindex(J, 0, lower);
		}
		++lower;
	}

	js_copy(J, 0);
}

static void Ap_shift(js_State *J)
{
	int k, len;

	len = js_getlength(J, 0);

	if (len == 0) {
		js_setlength(J, 0, 0);
		js_pushundefined(J);
		return;
	}

	js_getindex(J, 0, 0);

	for (k = 1; k < len; ++k) {
		if (js_hasindex(J, 0, k))
			js_setindex(J, 0, k - 1);
		else
			js_delindex(J, 0, k - 1);
	}

	js_delindex(J, 0, len - 1);
	js_setlength(J, 0, len - 1);
}

static void Ap_slice(js_State *J)
{
	int len, s, e, n;
	double sv, ev;

	js_newarray(J);

	len = js_getlength(J, 0);
	sv = js_tointeger(J, 1);
	ev = js_isdefined(J, 2) ? js_tointeger(J, 2) : len;

	if (sv < 0) sv = sv + len;
	if (ev < 0) ev = ev + len;

	s = sv < 0 ? 0 : sv > len ? len : sv;
	e = ev < 0 ? 0 : ev > len ? len : ev;

	for (n = 0; s < e; ++s, ++n)
		if (js_hasindex(J, 0, s))
			js_setindex(J, -2, n);
}

static int Ap_sort_cmp(js_State *J, int idx_a, int idx_b)
{
	js_Object *obj = js_tovalue(J, 0)->u.object;
	if (obj->u.a.simple) {
		js_Value *val_a = &obj->u.a.array[idx_a];
		js_Value *val_b = &obj->u.a.array[idx_b];
		int und_a = val_a->t.type == JS_TUNDEFINED;
		int und_b = val_b->t.type == JS_TUNDEFINED;
		if (und_a) return und_b;
		if (und_b) return -1;
		if (js_iscallable(J, 1)) {
			double v;
			js_copy(J, 1); /* copy function */
			js_pushundefined(J); /* no 'this' binding */
			js_pushvalue(J, *val_a);
			js_pushvalue(J, *val_b);
			js_call(J, 2);
			v = js_tonumber(J, -1);
			js_pop(J, 1);
			if (isnan(v))
				return 0;
			if (v == 0)
				return 0;
			return v < 0 ? -1 : 1;
		} else {
			const char *str_a, *str_b;
			int c;
			js_pushvalue(J, *val_a);
			js_pushvalue(J, *val_b);
			str_a = js_tostring(J, -2);
			str_b = js_tostring(J, -1);
			c = strcmp(str_a, str_b);
			js_pop(J, 2);
			return c;
		}
	} else {
		int und_a, und_b;
		int has_a = js_hasindex(J, 0, idx_a);
		int has_b = js_hasindex(J, 0, idx_b);
		if (!has_a && !has_b) {
			return 0;
		}
		if (has_a && !has_b) {
			js_pop(J, 1);
			return -1;
		}
		if (!has_a && has_b) {
			js_pop(J, 1);
			return 1;
		}

		und_a = js_isundefined(J, -2);
		und_b = js_isundefined(J, -1);
		if (und_a) {
			js_pop(J, 2);
			return und_b;
		}
		if (und_b) {
			js_pop(J, 2);
			return -1;
		}

		if (js_iscallable(J, 1)) {
			double v;
			js_copy(J, 1); /* copy function */
			js_pushundefined(J); /* no 'this' binding */
			js_copy(J, -4);
			js_copy(J, -4);
			js_call(J, 2);
			v = js_tonumber(J, -1);
			js_pop(J, 3);
			if (isnan(v))
				return 0;
			if (v == 0)
				return 0;
			return v < 0 ? -1 : 1;
		} else {
			const char *str_a = js_tostring(J, -2);
			const char *str_b = js_tostring(J, -1);
			int c = strcmp(str_a, str_b);
			js_pop(J, 2);
			return c;
		}
	}
}

static void Ap_sort_swap(js_State *J, int idx_a, int idx_b)
{
	js_Object *obj = js_tovalue(J, 0)->u.object;
	if (obj->u.a.simple) {
		js_Value tmp = obj->u.a.array[idx_a];
		obj->u.a.array[idx_a] = obj->u.a.array[idx_b];
		obj->u.a.array[idx_b] = tmp;
	} else {
		int has_a = js_hasindex(J, 0, idx_a);
		int has_b = js_hasindex(J, 0, idx_b);
		if (has_a && has_b) {
			js_setindex(J, 0, idx_a);
			js_setindex(J, 0, idx_b);
		} else if (has_a && !has_b) {
			js_delindex(J, 0, idx_a);
			js_setindex(J, 0, idx_b);
		} else if (!has_a && has_b) {
			js_delindex(J, 0, idx_b);
			js_setindex(J, 0, idx_a);
		}
	}
}

/* A bottom-up/bouncing heapsort implementation */

static int Ap_sort_leaf(js_State *J, int i, int end)
{
	int j = i;
	int lc = (j << 1) + 1; /* left child */
	int rc = (j << 1) + 2; /* right child */
	while (rc < end) {
		if (Ap_sort_cmp(J, rc, lc) > 0)
			j = rc;
		else
			j = lc;
		lc = (j << 1) + 1;
		rc = (j << 1) + 2;
	}
	if (lc < end)
		j = lc;
	return j;
}

static void Ap_sort_sift(js_State *J, int i, int end)
{
	int j = Ap_sort_leaf(J, i, end);
	while (Ap_sort_cmp(J, i, j) > 0)
		j = (j - 1) >> 1; /* parent */
	while (j > i) {
		Ap_sort_swap(J, i, j);
		j = (j - 1) >> 1; /* parent */
	}
}

static void Ap_sort_heapsort(js_State *J, int n)
{
	int i;
	for (i = n / 2 - 1; i >= 0; --i)
		Ap_sort_sift(J, i, n);
	for (i = n - 1; i > 0; --i) {
		Ap_sort_swap(J, 0, i);
		Ap_sort_sift(J, 0, i);
	}
}

static void Ap_sort(js_State *J)
{
	int len;

	len = js_getlength(J, 0);
	if (len <= 1) {
		js_copy(J, 0);
		return;
	}

	if (!js_iscallable(J, 1) && !js_isundefined(J, 1))
		js_typeerror(J, "comparison function must be a function or undefined");

	if (len >= INT_MAX)
		js_rangeerror(J, "array is too large to sort");

	Ap_sort_heapsort(J, len);

	js_copy(J, 0);
}

static void Ap_splice(js_State *J)
{
	int top = js_gettop(J);
	int len, start, del, add, k;

	len = js_getlength(J, 0);
	start = js_tointeger(J, 1);
	if (start < 0)
		start = (len + start) > 0 ? len + start : 0;
	else if (start > len)
		start = len;

	if (js_isdefined(J, 2))
		del = js_tointeger(J, 2);
	else
		del = len - start;
	if (del > len - start)
		del = len - start;
	if (del < 0)
		del = 0;

	js_newarray(J);

	/* copy deleted items to return array */
	for (k = 0; k < del; ++k)
		if (js_hasindex(J, 0, start + k))
			js_setindex(J, -2, k);
	js_setlength(J, -1, del);

	/* shift the tail to resize the hole left by deleted items */
	add = top - 3;
	if (add < del) {
		for (k = start; k < len - del; ++k) {
			if (js_hasindex(J, 0, k + del))
				js_setindex(J, 0, k + add);
			else
				js_delindex(J, 0, k + add);
		}
		for (k = len; k > len - del + add; --k)
			js_delindex(J, 0, k - 1);
	} else if (add > del) {
		for (k = len - del; k > start; --k) {
			if (js_hasindex(J, 0, k + del - 1))
				js_setindex(J, 0, k + add - 1);
			else
				js_delindex(J, 0, k + add - 1);
		}
	}

	/* copy new items into the hole */
	for (k = 0; k < add; ++k) {
		js_copy(J, 3 + k);
		js_setindex(J, 0, start + k);
	}

	js_setlength(J, 0, len - del + add);
}

static void Ap_unshift(js_State *J)
{
	int i, top = js_gettop(J);
	int k, len;

	len = js_getlength(J, 0);

	for (k = len; k > 0; --k) {
		int from = k - 1;
		int to = k + top - 2;
		if (js_hasindex(J, 0, from))
			js_setindex(J, 0, to);
		else
			js_delindex(J, 0, to);
	}

	for (i = 1; i < top; ++i) {
		js_copy(J, i);
		js_setindex(J, 0, i - 1);
	}

	js_setlength(J, 0, len + top - 1);

	js_pushnumber(J, len + top - 1);
}

static void Ap_toString(js_State *J)
{
	if (!js_iscoercible(J, 0))
		js_typeerror(J, "'this' is not an object");
	js_getproperty(J, 0, "join");
	if (!js_iscallable(J, -1)) {
		js_pop(J, 1);
		/* TODO: call Object.prototype.toString implementation directly */
		js_getglobal(J, "Object");
		js_getproperty(J, -1, "prototype");
		js_rot2pop1(J);
		js_getproperty(J, -1, "toString");
		js_rot2pop1(J);
	}
	js_copy(J, 0);
	js_call(J, 0);
}

static void Ap_indexOf(js_State *J)
{
	int k, len, from;

	len = js_getlength(J, 0);
	from = js_isdefined(J, 2) ? js_tointeger(J, 2) : 0;
	if (from < 0) from = len + from;
	if (from < 0) from = 0;

	js_copy(J, 1);
	for (k = from; k < len; ++k) {
		if (js_hasindex(J, 0, k)) {
			if (js_strictequal(J)) {
				js_pushnumber(J, k);
				return;
			}
			js_pop(J, 1);
		}
	}

	js_pushnumber(J, -1);
}

static void Ap_includes(js_State *J)
{
	int k, len, from;

	len = js_getlength(J, 0);
	from = js_isdefined(J, 2) ? js_tointeger(J, 2) : 0;
	if (from < 0) from = len + from;
	if (from < 0) from = 0;

	js_copy(J, 1);
	for (k = from; k < len; ++k) {
		if (js_hasindex(J, 0, k)) {
			if (js_strictequal(J)) {
				js_pushboolean(J, 1);
				return;
			}
			js_pop(J, 1);
		}
	}

	js_pushboolean(J, 0);
}

static void Ap_lastIndexOf(js_State *J)
{
	int k, len, from;

	len = js_getlength(J, 0);
	from = js_isdefined(J, 2) ? js_tointeger(J, 2) : len - 1;
	if (from > len - 1) from = len - 1;
	if (from < 0) from = len + from;

	js_copy(J, 1);
	for (k = from; k >= 0; --k) {
		if (js_hasindex(J, 0, k)) {
			if (js_strictequal(J)) {
				js_pushnumber(J, k);
				return;
			}
			js_pop(J, 1);
		}
	}

	js_pushnumber(J, -1);
}

static void Ap_every(js_State *J)
{
	int hasthis = js_gettop(J) >= 3;
	int k, len;

	if (!js_iscallable(J, 1))
		js_typeerror(J, "callback is not a function");

	len = js_getlength(J, 0);
	for (k = 0; k < len; ++k) {
		if (js_hasindex(J, 0, k)) {
			js_copy(J, 1);
			if (hasthis)
				js_copy(J, 2);
			else
				js_pushundefined(J);
			js_copy(J, -3);
			js_pushnumber(J, k);
			js_copy(J, 0);
			js_call(J, 3);
			if (!js_toboolean(J, -1))
				return;
			js_pop(J, 2);
		}
	}

	js_pushboolean(J, 1);
}

static void Ap_some(js_State *J)
{
	int hasthis = js_gettop(J) >= 3;
	int k, len;

	if (!js_iscallable(J, 1))
		js_typeerror(J, "callback is not a function");

	len = js_getlength(J, 0);
	for (k = 0; k < len; ++k) {
		if (js_hasindex(J, 0, k)) {
			js_copy(J, 1);
			if (hasthis)
				js_copy(J, 2);
			else
				js_pushundefined(J);
			js_copy(J, -3);
			js_pushnumber(J, k);
			js_copy(J, 0);
			js_call(J, 3);
			if (js_toboolean(J, -1))
				return;
			js_pop(J, 2);
		}
	}

	js_pushboolean(J, 0);
}

static void Ap_forEach(js_State *J)
{
	int hasthis = js_gettop(J) >= 3;
	int k, len;

	if (!js_iscallable(J, 1))
		js_typeerror(J, "callback is not a function");

	len = js_getlength(J, 0);
	for (k = 0; k < len; ++k) {
		if (js_hasindex(J, 0, k)) {
			js_copy(J, 1);
			if (hasthis)
				js_copy(J, 2);
			else
				js_pushundefined(J);
			js_copy(J, -3);
			js_pushnumber(J, k);
			js_copy(J, 0);
			js_call(J, 3);
			js_pop(J, 2);
		}
	}

	js_pushundefined(J);
}

static void Ap_map(js_State *J)
{
	int hasthis = js_gettop(J) >= 3;
	int k, len;

	if (!js_iscallable(J, 1))
		js_typeerror(J, "callback is not a function");

	js_newarray(J);

	len = js_getlength(J, 0);
	for (k = 0; k < len; ++k) {
		if (js_hasindex(J, 0, k)) {
			js_copy(J, 1);
			if (hasthis)
				js_copy(J, 2);
			else
				js_pushundefined(J);
			js_copy(J, -3);
			js_pushnumber(J, k);
			js_copy(J, 0);
			js_call(J, 3);
			js_setindex(J, -3, k);
			js_pop(J, 1);
		}
	}
	js_setlength(J, -1, len);
}

static void Ap_filter(js_State *J)
{
	int hasthis = js_gettop(J) >= 3;
	int k, to, len;

	if (!js_iscallable(J, 1))
		js_typeerror(J, "callback is not a function");

	js_newarray(J);
	to = 0;

	len = js_getlength(J, 0);
	for (k = 0; k < len; ++k) {
		if (js_hasindex(J, 0, k)) {
			js_copy(J, 1);
			if (hasthis)
				js_copy(J, 2);
			else
				js_pushundefined(J);
			js_copy(J, -3);
			js_pushnumber(J, k);
			js_copy(J, 0);
			js_call(J, 3);
			if (js_toboolean(J, -1)) {
				js_pop(J, 1);
				js_setindex(J, -2, to++);
			} else {
				js_pop(J, 2);
			}
		}
	}
}

static void Ap_reduce(js_State *J)
{
	int hasinitial = js_gettop(J) >= 3;
	int k, len;

	if (!js_iscallable(J, 1))
		js_typeerror(J, "callback is not a function");

	len = js_getlength(J, 0);
	k = 0;

	if (len == 0 && !hasinitial)
		js_typeerror(J, "no initial value");

	/* initial value of accumulator */
	if (hasinitial)
		js_copy(J, 2);
	else {
		while (k < len)
			if (js_hasindex(J, 0, k++))
				break;
		if (k == len)
			js_typeerror(J, "no initial value");
	}

	while (k < len) {
		if (js_hasindex(J, 0, k)) {
			js_copy(J, 1);
			js_pushundefined(J);
			js_rot(J, 4); /* accumulator on top */
			js_rot(J, 4); /* property on top */
			js_pushnumber(J, k);
			js_copy(J, 0);
			js_call(J, 4); /* calculate new accumulator */
		}
		++k;
	}

	/* return accumulator */
}

static void Ap_reduceRight(js_State *J)
{
	int hasinitial = js_gettop(J) >= 3;
	int k, len;

	if (!js_iscallable(J, 1))
		js_typeerror(J, "callback is not a function");

	len = js_getlength(J, 0);
	k = len - 1;

	if (len == 0 && !hasinitial)
		js_typeerror(J, "no initial value");

	/* initial value of accumulator */
	if (hasinitial)
		js_copy(J, 2);
	else {
		while (k >= 0)
			if (js_hasindex(J, 0, k--))
				break;
		if (k < 0)
			js_typeerror(J, "no initial value");
	}

	while (k >= 0) {
		if (js_hasindex(J, 0, k)) {
			js_copy(J, 1);
			js_pushundefined(J);
			js_rot(J, 4); /* accumulator on top */
			js_rot(J, 4); /* property on top */
			js_pushnumber(J, k);
			js_copy(J, 0);
			js_call(J, 4); /* calculate new accumulator */
		}
		--k;
	}

	/* return accumulator */
}

static void A_isArray(js_State *J)
{
	if (js_isobject(J, 1)) {
		js_Object *T = js_toobject(J, 1);
		js_pushboolean(J, T->type == JS_CARRAY);
	} else {
		js_pushboolean(J, 0);
	}
}

void jsB_initarray(js_State *J)
{
	js_pushobject(J, J->Array_prototype);
	{
		jsB_propf(J, "Array.prototype.toString", Ap_toString, 0);
		jsB_propf(J, "Array.prototype.concat", Ap_concat, 0); /* 1 */
		jsB_propf(J, "Array.prototype.join", Ap_join, 1);
		jsB_propf(J, "Array.prototype.pop", Ap_pop, 0);
		jsB_propf(J, "Array.prototype.push", Ap_push, 0); /* 1 */
		jsB_propf(J, "Array.prototype.reverse", Ap_reverse, 0);
		jsB_propf(J, "Array.prototype.shift", Ap_shift, 0);
		jsB_propf(J, "Array.prototype.slice", Ap_slice, 2);
		jsB_propf(J, "Array.prototype.sort", Ap_sort, 1);
		jsB_propf(J, "Array.prototype.splice", Ap_splice, 2);
		jsB_propf(J, "Array.prototype.unshift", Ap_unshift, 0); /* 1 */

		/* ES5 */
		jsB_propf(J, "Array.prototype.indexOf", Ap_indexOf, 1);
		jsB_propf(J, "Array.prototype.lastIndexOf", Ap_lastIndexOf, 1);
		jsB_propf(J, "Array.prototype.every", Ap_every, 1);
		jsB_propf(J, "Array.prototype.some", Ap_some, 1);
		jsB_propf(J, "Array.prototype.forEach", Ap_forEach, 1);
		jsB_propf(J, "Array.prototype.map", Ap_map, 1);
		jsB_propf(J, "Array.prototype.filter", Ap_filter, 1);
		jsB_propf(J, "Array.prototype.reduce", Ap_reduce, 1);
		jsB_propf(J, "Array.prototype.reduceRight", Ap_reduceRight, 1);
		
		/* ES7 */
		jsB_propf(J, "Array.prototype.includes", Ap_includes, 2);
	}
	js_newcconstructor(J, jsB_new_Array, jsB_new_Array, "Array", 0); /* 1 */
	{
		/* ES5 */
		jsB_propf(J, "Array.isArray", A_isArray, 1);
	}
	js_defglobal(J, "Array", JS_DONTENUM);
}
