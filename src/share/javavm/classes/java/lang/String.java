/*
 * @(#)String.java	1.145 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER  
 *   
 * This program is free software; you can redistribute it and/or  
 * modify it under the terms of the GNU General Public License version  
 * 2 only, as published by the Free Software Foundation.   
 *   
 * This program is distributed in the hope that it will be useful, but  
 * WITHOUT ANY WARRANTY; without even the implied warranty of  
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  
 * General Public License version 2 for more details (a copy is  
 * included at /legal/license.txt).   
 *   
 * You should have received a copy of the GNU General Public License  
 * version 2 along with this work; if not, write to the Free Software  
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  
 * 02110-1301 USA   
 *   
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa  
 * Clara, CA 95054 or visit www.sun.com if you need additional  
 * information or have any questions. 
 *
 */

package java.lang;

import sun.misc.CVM;
import java.io.ObjectStreamClass;
import java.io.ObjectStreamField;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.Locale;


/**
 * The <code>String</code> class represents character strings. All
 * string literals in Java programs, such as <code>"abc"</code>, are
 * implemented as instances of this class.
 * <p>
 * Strings are constant; their values cannot be changed after they
 * are created. String buffers support mutable strings.
 * Because String objects are immutable they can be shared. For example:
 * <p><blockquote><pre>
 *     String str = "abc";
 * </pre></blockquote><p>
 * is equivalent to:
 * <p><blockquote><pre>
 *     char data[] = {'a', 'b', 'c'};
 *     String str = new String(data);
 * </pre></blockquote><p>
 * Here are some more examples of how strings can be used:
 * <p><blockquote><pre>
 *     System.out.println("abc");
 *     String cde = "cde";
 *     System.out.println("abc" + cde);
 *     String c = "abc".substring(2,3);
 *     String d = cde.substring(1, 2);
 * </pre></blockquote>
 * <p>
 * The class <code>String</code> includes methods for examining
 * individual characters of the sequence, for comparing strings, for
 * searching strings, for extracting substrings, and for creating a
 * copy of a string with all characters translated to uppercase or to
 * lowercase. Case mapping relies heavily on the information provided
 * by the Unicode Consortium's Unicode 3.0 specification. The
 * specification's UnicodeData.txt and SpecialCasing.txt files are
 * used extensively to provide case mapping.
 * <p>
 * The Java language provides special support for the string
 * concatenation operator (&nbsp;+&nbsp;), and for conversion of
 * other objects to strings. String concatenation is implemented
 * through the <code>StringBuffer</code> class and its
 * <code>append</code> method.
 * String conversions are implemented through the method
 * <code>toString</code>, defined by <code>Object</code> and
 * inherited by all classes in Java. For additional information on
 * string concatenation and conversion, see Gosling, Joy, and Steele,
 * <i>The Java Language Specification</i>.
 *
 * <p> Unless otherwise noted, passing a <tt>null</tt> argument to a constructor
 * or method in this class will cause a {@link NullPointerException} to be
 * thrown.
 *
 * @author  Lee Boynton
 * @author  Arthur van Hoff
 * @version 1.152, 02/01/03
 * @see     java.lang.Object#toString()
 * @see     java.lang.StringBuffer
 * @see     java.lang.StringBuffer#append(boolean)
 * @see     java.lang.StringBuffer#append(char)
 * @see     java.lang.StringBuffer#append(char[])
 * @see     java.lang.StringBuffer#append(char[], int, int)
 * @see     java.lang.StringBuffer#append(double)
 * @see     java.lang.StringBuffer#append(float)
 * @see     java.lang.StringBuffer#append(int)
 * @see     java.lang.StringBuffer#append(long)
 * @see     java.lang.StringBuffer#append(java.lang.Object)
 * @see     java.lang.StringBuffer#append(java.lang.String)

 * @since   JDK1.0
 */

public final class String
    implements java.io.Serializable, Comparable, CharSequence
{
    /** The value is used for character storage. */
    private char value[];

    /** The offset is the first index of the storage that is used. */
    private int offset;

    /** The count is the number of characters in the String. */
    private int count;

    /** use serialVersionUID from JDK 1.0.2 for interoperability */
    private static final long serialVersionUID = -6849794470754667710L;

    /**
     * Class String is special cased within the Serialization Stream Protocol.
     *
     * A String instance is written intially into an ObjectOutputStream in the
     * following format:
     * <pre>
     *      <code>TC_STRING</code> (utf String)
     * </pre>
     * The String is written by method <code>DataOutput.writeUTF</code>.
     * A new handle is generated to  refer to all future references to the
     * string instance within the stream.
     */
    private static final ObjectStreamField[] serialPersistentFields =
        new ObjectStreamField[0];

    /**
     * Initializes a newly created <code>String</code> object so that it
     * represents an empty character sequence.  Note that use of this 
     * constructor is unnecessary since Strings are immutable. 
     */
    public String() {
        value = new char[0];
    }

    /**
     * Initializes a newly created <code>String</code> object so that it
     * represents the same sequence of characters as the argument; in other
     * words, the newly created string is a copy of the argument string. Unless 
     * an explicit copy of <code>original</code> is needed, use of this 
     * constructor is unnecessary since Strings are immutable. 
     *
     * @param   original   a <code>String</code>.
     */
     public String(String original) {
        this.count = original.count;
        if (original.value.length > this.count) {
            // The array representing the String is bigger than the new
            // String itself.  Perhaps this constructor is being called
            // in order to trim the baggage, so make a copy of the array.
            this.value = new char[this.count];
            CVM.copyCharArray(original.value, original.offset,
                              this.value, 0, this.count);
        } else {
            // The array representing the String is the same
            // size as the String, so no point in making a copy.
            this.value = original.value;
        }
    }
     
    /**
     * Allocates a new <code>String</code> so that it represents the
     * sequence of characters currently contained in the character array
     * argument. The contents of the character array are copied; subsequent
     * modification of the character array does not affect the newly created
     * string.
     *
     * @param  value   the initial value of the string.
     */
    public String(char value[]) {
        this.count = value.length;
        this.value = new char[count];
        CVM.copyCharArray(value, 0, this.value, 0, count);
    }

    /**
     * Allocates a new <code>String</code> that contains characters from
     * a subarray of the character array argument. The <code>offset</code>
     * argument is the index of the first character of the subarray and
     * the <code>count</code> argument specifies the length of the
     * subarray. The contents of the subarray are copied; subsequent
     * modification of the character array does not affect the newly
     * created string.
     *
     * @param      value    array that is the source of characters.
     * @param      offset   the initial offset.
     * @param      count    the length.
     * @exception  IndexOutOfBoundsException  if the <code>offset</code>
     *               and <code>count</code> arguments index characters outside
     *               the bounds of the <code>value</code> array.
     */
    public String(char value[], int offset, int count) {
        if (offset < 0) {
            throw new StringIndexOutOfBoundsException(offset);
        }
        if (count < 0) {
            throw new StringIndexOutOfBoundsException(count);
        }
        // Note: offset or count might be near -1>>>1.
        if (offset > value.length - count) {
            throw new StringIndexOutOfBoundsException(offset + count);
        }

        this.value = new char[count];
        this.count = count;
        CVM.copyCharArray(value, offset, this.value, 0, count);
    }

    /**
     * Allocates a new <code>String</code> constructed from a subarray
     * of an array of 8-bit integer values.
     * <p>
     * The <code>offset</code> argument is the index of the first byte
     * of the subarray, and the <code>count</code> argument specifies the
     * length of the subarray.
     * <p>
     * Each <code>byte</code> in the subarray is converted to a
     * <code>char</code> as specified in the method above.
     *
     * deprecated This method does not properly convert bytes into characters.
     * As of JDK&nbsp;1.1, the preferred way to do this is via the
     * <code>String</code> constructors that take a charset name or that use
     * the platform's default charset.
     *
     * param      ascii     the bytes to be converted to characters.
     * param      hibyte    the top 8 bits of each 16-bit Unicode character.
     * param      offset    the initial offset.
     * param      count     the length.
     * exception  IndexOutOfBoundsException  if the <code>offset</code>
     *               or <code>count</code> argument is invalid.
     * @see        java.lang.String#String(byte[], int)
     * @see        java.lang.String#String(byte[], int, int, java.lang.String)
     * @see        java.lang.String#String(byte[], int, int)
     * @see        java.lang.String#String(byte[], java.lang.String)
     * @see        java.lang.String#String(byte[])
     */
    /*
    public String(byte ascii[], int hibyte, int offset, int count) {
	//checkBounds(ascii, offset, count);

        char value[] = new char[count];
        this.count = count;
        this.value = value;

        if (hibyte == 0) {
            for (int i = count ; i-- > 0 ;) {
                value[i] = (char) (ascii[i + offset] & 0xff);
            }
        } else {
            hibyte <<= 8;
            for (int i = count ; i-- > 0 ;) {
                value[i] = (char) (hibyte | (ascii[i + offset] & 0xff));
            }
        }
    }
    */

    /**
     * Allocates a new <code>String</code> containing characters
     * constructed from an array of 8-bit integer values. Each character
     * <i>c</i>in the resulting string is constructed from the
     * corresponding component <i>b</i> in the byte array such that:
     * <p><blockquote><pre>
     *     <b><i>c</i></b> == (char)(((hibyte &amp; 0xff) &lt;&lt; 8)
     *                         | (<b><i>b</i></b> &amp; 0xff))
     * </pre></blockquote>
     *
     * deprecated This method does not properly convert bytes into characters.
     * As of JDK&nbsp;1.1, the preferred way to do this is via the
     * <code>String</code> constructors that take a charset name or
     * that use the platform's default charset.
     *
     * @param      ascii    the bytes to be converted to characters.
     * @param      hibyte   the top 8 bits of each 16-bit Unicode character.
     * @see        java.lang.String#String(byte[], int, int, java.lang.String)
     * @see        java.lang.String#String(byte[], int, int)
     * @see        java.lang.String#String(byte[], java.lang.String)
     * @see        java.lang.String#String(byte[])
     */
    /*
    public String(byte ascii[], int hibyte) {
        this(ascii, hibyte, 0, ascii.length);
    }
     */

    /* Common private utility method used to bounds check the byte array
     * and requested offset & length values used by the String(byte[],..)
     * constructors.
     */
    private static void checkBounds(byte[] bytes, int offset, int length) {
        if (length < 0)
            throw new StringIndexOutOfBoundsException(length);
        if (offset < 0)
            throw new StringIndexOutOfBoundsException(offset);
        if (offset > bytes.length - length)
            throw new StringIndexOutOfBoundsException(offset + length);
    }


    /**
     * Constructs a new <tt>String</tt> by decoding the specified subarray of
     * bytes using the specified charset.  The length of the new
     * <tt>String</tt> is a function of the charset, and hence may not be equal
     * to the length of the subarray.
     *
     * <p> The behavior of this constructor when the given bytes are not valid
     * in the given charset is unspecified.  
     *
     * @param  bytes   the bytes to be decoded into characters
     * @param  offset  the index of the first byte to decode
     * @param  length  the number of bytes to decode
     * @param  charsetName  the name of a supported encoding
     *
     * @throws  UnsupportedEncodingException
     *          if the named charset is not supported
     * @throws  IndexOutOfBoundsException
     *          if the <tt>offset</tt> and <tt>length</tt> arguments
     *          index characters outside the bounds of the <tt>bytes</tt>
     *          array
     * @since JDK1.1
     */
    public String(byte bytes[], int offset, int length, String charsetName)
	throws UnsupportedEncodingException
    {
	if (charsetName == null)
	    throw new NullPointerException("charsetName");
	checkBounds(bytes, offset, length);
	value = StringCoding.decode(charsetName, bytes, offset, length);
	count = value.length;
    }

    /**
     * Constructs a new <tt>String</tt> by decoding the specified array of
     * bytes using the specified charset.  The length of the new
     * <tt>String</tt> is a function of the charset, and hence may not be equal
     * to the length of the byte array.
     *
     * <p> The behavior of this constructor when the given bytes are not valid
     * in the given charset is unspecified.  
     *
     * @param  bytes   the bytes to be decoded into characters
     * @param  charsetName  the name of a supported encoding
     *
     * @exception  UnsupportedEncodingException
     *             If the named charset is not supported
     * @since      JDK1.1
     */
    public String(byte bytes[], String charsetName)
	throws UnsupportedEncodingException
    {
	this(bytes, 0, bytes.length, charsetName);
    }

    /**
     * Constructs a new <tt>String</tt> by decoding the specified subarray of
     * bytes using the platform's default charset.  The length of the new
     * <tt>String</tt> is a function of the charset, and hence may not be equal
     * to the length of the subarray.
     *
     * <p> The behavior of this constructor when the given bytes are not valid
     * in the default charset is unspecified.  
     *
     * @param  bytes   the bytes to be decoded into characters
     * @param  offset  the index of the first byte to decode
     * @param  length  the number of bytes to decode
     * @throws IndexOutOfBoundsException
     *         if the <code>offset</code> and the <code>length</code>
     *         arguments index characters outside the bounds of the
     *         <code>bytes</code> array
     * @since  JDK1.1
     */
    public String(byte bytes[], int offset, int length) {
        checkBounds(bytes, offset, length);
        value = StringCoding.decode(bytes, offset, length);
        count = value.length;
    }

    /**
     * Constructs a new <tt>String</tt> by decoding the specified array of
     * bytes using the platform's default charset.  The length of the new
     * <tt>String</tt> is a function of the charset, and hence may not be equal
     * to the length of the byte array.
     *
     * <p> The behavior of this constructor when the given bytes are not valid
     * in the default charset is unspecified. 
     *
     * @param  bytes   the bytes to be decoded into characters
     * @since  JDK1.1
     */
    public String(byte bytes[]) {
	this(bytes, 0, bytes.length);
    }

    /**
     * Allocates a new string that contains the sequence of characters
     * currently contained in the string buffer argument. The contents of
     * the string buffer are copied; subsequent modification of the string
     * buffer does not affect the newly created string.
     *
     * @param   buffer   a <code>StringBuffer</code>.
     */
    public String (StringBuffer buffer) {
        synchronized(buffer) {
            buffer.setShared();
            this.value = buffer.getValue();
            this.offset = 0;
            this.count = buffer.lengthNoSync();
        }
    }

    private void initSimpleSync(StringBuffer buffer) {
 	if (buffer != null && CVM.simpleLockGrab(buffer)) {
	    /* Note, method calls are made below, which is usualy a no no
	     * after calling simpleLockGrab(). However, this method is marked
	     * as CVMJIT_NEEDS_TO_INLINE, which will only allow this method
	     * to be compiled if the simple method calls below can also
	     * be inlined.
	     */
            buffer.setShared();
            this.value = buffer.getValue();
            this.offset = 0;
            this.count = buffer.lengthNoSync();
	    CVM.simpleLockRelease(buffer);
	    return;
        }
	initSync(buffer);
    }
    private void initSync(StringBuffer buffer) {
        synchronized(buffer) {
            buffer.setShared();
            this.value = buffer.getValue();
            this.offset = 0;
            this.count = buffer.lengthNoSync();
        }
    }

    // Package private constructor which shares value array for speed.
    String(int offset, int count, char value[]) {
	this.value = value;
	this.offset = offset;
	this.count = count;
    }

    /**
     * Returns the length of this string.
     * The length is equal to the number of 16-bit
     * Unicode characters in the string.
     *
     * @return  the length of the sequence of characters represented by this
     *          object.
     */
    public int length() {
        return count;
    }

    /**
     * Returns the character at the specified index. An index ranges
     * from <code>0</code> to <code>length() - 1</code>. The first character
     * of the sequence is at index <code>0</code>, the next at index
     * <code>1</code>, and so on, as for array indexing.
     *
     * @param      index   the index of the character.
     * @return     the character at the specified index of this string.
     *             The first character is at index <code>0</code>.
     * @exception  IndexOutOfBoundsException  if the <code>index</code>
     *             argument is negative or not less than the length of this
     *             string.
     */
    public native char charAt(int index);

    /*
     * The original Java version
     *
     * public char charAt(int index) {
     *	if ((index < 0) || (index >= count)) {
     *	    throw new StringIndexOutOfBoundsException(index);
     *	}
     *	return value[index + offset];
     *}
     */

    /**
     * Copies characters from this string into the destination character
     * array.
     * <p>
     * The first character to be copied is at index <code>srcBegin</code>;
     * the last character to be copied is at index <code>srcEnd-1</code>
     * (thus the total number of characters to be copied is
     * <code>srcEnd-srcBegin</code>). The characters are copied into the
     * subarray of <code>dst</code> starting at index <code>dstBegin</code>
     * and ending at index:
     * <p><blockquote><pre>
     *     dstbegin + (srcEnd-srcBegin) - 1
     * </pre></blockquote>
     *
     * @param      srcBegin   index of the first character in the string
     *                        to copy.
     * @param      srcEnd     index after the last character in the string
     *                        to copy.
     * @param      dst        the destination array.
     * @param      dstBegin   the start offset in the destination array.
     * @exception IndexOutOfBoundsException If any of the following
     *            is true:
     *            <ul><li><code>srcBegin</code> is negative.
     *            <li><code>srcBegin</code> is greater than <code>srcEnd</code>
     *            <li><code>srcEnd</code> is greater than the length of this
     *                string
     *            <li><code>dstBegin</code> is negative
     *            <li><code>dstBegin+(srcEnd-srcBegin)</code> is larger than
     *                <code>dst.length</code></ul>
     */
    public native void getChars(int srcBegin, int srcEnd, 
                                char dst[], int dstBegin);
    /*
     * The original Java version.
     *
     * public void getChars(int srcBegin, int srcEnd, char dst[], int dstBegin) {
     *	if (srcBegin < 0) {
     *	    throw new StringIndexOutOfBoundsException(srcBegin);
     *	} 
     *	if (srcEnd > count) {
     *	    throw new StringIndexOutOfBoundsException(srcEnd);
     *	} 
     *	if (srcBegin > srcEnd) {
     *	    throw new StringIndexOutOfBoundsException(srcEnd - srcBegin);
     *	}
     *	System.arraycopy(value, offset + srcBegin, dst, dstBegin, 
     *			 srcEnd - srcBegin);
     * }
     */

    /**
     * Copies characters from this string into the destination byte
     * array. Each byte receives the 8 low-order bits of the
     * corresponding character. The eight high-order bits of each character
     * are not copied and do not participate in the transfer in any way.
     * <p>
     * The first character to be copied is at index <code>srcBegin</code>;
     * the last character to be copied is at index <code>srcEnd-1</code>.
     * The total number of characters to be copied is
     * <code>srcEnd-srcBegin</code>. The characters, converted to bytes,
     * are copied into the subarray of <code>dst</code> starting at index
     * <code>dstBegin</code> and ending at index:
     * <p><blockquote><pre>
     *     dstbegin + (srcEnd-srcBegin) - 1
     * </pre></blockquote>
     *
     * deprecated This method does not properly convert characters into bytes.
     * As of JDK&nbsp;1.1, the preferred way to do this is via the
     * the <code>getBytes()</code> method, which uses the platform's default
     * charset.
     *
     * @param      srcBegin   index of the first character in the string
     *                        to copy.
     * @param      srcEnd     index after the last character in the string
     *                        to copy.
     * @param      dst        the destination array.
     * @param      dstBegin   the start offset in the destination array.
     * @exception IndexOutOfBoundsException if any of the following
     *            is true:
     *           <ul><li><code>srcBegin</code> is negative
     *           <li><code>srcBegin</code> is greater than <code>srcEnd</code>
     *           <li><code>srcEnd</code> is greater than the length of this
     *            String
     *           <li><code>dstBegin</code> is negative
     *           <li><code>dstBegin+(srcEnd-srcBegin)</code> is larger than
     *            <code>dst.length</code></ul>
     *
    public void getBytes(int srcBegin, int srcEnd, byte dst[], int dstBegin) {
        if (srcBegin < 0) {
            throw new StringIndexOutOfBoundsException(srcBegin);
        }
        if (srcEnd > count) {
            throw new StringIndexOutOfBoundsException(srcEnd);
        }
        if (srcBegin > srcEnd) {
            throw new StringIndexOutOfBoundsException(srcEnd - srcBegin);
        }
        int j = dstBegin;
        int n = offset + srcEnd;
        int i = offset + srcBegin;
	char[] val = value;   /* avoid getfield opcode *

        while (i < n) {
            dst[j++] = (byte)val[i++];
        }
    }
    */

    /**
     * Encodes this <tt>String</tt> into a sequence of bytes using the
     * named charset, storing the result into a new byte array.
     *
     * <p> The behavior of this method when this string cannot be encoded in
     * the given charset is unspecified.  
     *
     * @param  charsetName
     *         the name of a supported encoding
     *
     * @return  The resultant byte array
     *
     * @exception  UnsupportedEncodingException
     *             If the named charset is not supported
     *
     * @since      JDK1.1
     */
    public byte[] getBytes(String charsetName)
	throws UnsupportedEncodingException
    {
	return StringCoding.encode(charsetName, value, offset, count);
    }

    /**
     * Encodes this <tt>String</tt> into a sequence of bytes using the
     * platform's default charset, storing the result into a new byte array.
     *
     * <p> The behavior of this method when this string cannot be encoded in
     * the default charset is unspecified.  
     *
     * @return  The resultant byte array
     *
     * @since      JDK1.1
     */
    public byte[] getBytes() {
	return StringCoding.encode(value, offset, count);
    }

    /**
     * Compares this string to the specified object.
     * The result is <code>true</code> if and only if the argument is not
     * <code>null</code> and is a <code>String</code> object that represents
     * the same sequence of characters as this object.
     *
     * @param   anObject   the object to compare this <code>String</code>
     *                     against.
     * @return  <code>true</code> if the <code>String </code>are equal;
     *          <code>false</code> otherwise.
     * @see     java.lang.String#compareTo(java.lang.String)
     * @see     java.lang.String#equalsIgnoreCase(java.lang.String)
     */
    public native boolean equals(Object anObject);

    /*
     * The original Java version
     *
     *public boolean equals(Object anObject) {
     *	if (this == anObject) {
     *	    return true;
     *	}
     *	if (anObject instanceof String) {
     *	    String anotherString = (String)anObject;
     *	    int n = count;
     *	    if (n == anotherString.count) {
     *		char v1[] = value;
     *		char v2[] = anotherString.value;
     *		int i = offset;
     *		int j = anotherString.offset;
     *		while (n-- != 0) {
     *		    if (v1[i++] != v2[j++]) {
     *			return false;
     *		    }
     *		}
     *		return true;
     *	    }
     *	}
     *	return false;
     *}
     */

    /**
     * Returns <tt>true</tt> if and only if this <tt>String</tt> represents
     * the same sequence of characters as the specified <tt>StringBuffer</tt>.
     *
     * @param   sb         the <tt>StringBuffer</tt> to compare to.
     * @return  <tt>true</tt> if and only if this <tt>String</tt> represents
     *          the same sequence of characters as the specified
     *          <tt>StringBuffer</tt>, otherwise <tt>false</tt>.
     * @since 1.4
     */
    public boolean contentEquals(StringBuffer sb) {
        synchronized(sb) {
            if (count != sb.length())
                return false;
            char v1[] = value;
            char v2[] = sb.getValue();
            int i = offset;
            int j = 0;
            int n = count;
            while (n-- != 0) {
                if (v1[i++] != v2[j++])
                    return false;
            }
        }
        return true;
    }

    /*
     * The original Java version
     *
     *public boolean equals(Object anObject) {
     *	if (this == anObject) {
     *	    return true;
     *	}
     *	if (anObject instanceof String) {
     *	    String anotherString = (String)anObject;
     *	    int n = count;
     *	    if (n == anotherString.count) {
     *		char v1[] = value;
     *		char v2[] = anotherString.value;
     *		int i = offset;
     *		int j = anotherString.offset;
     *		while (n-- != 0) {
     *		    if (v1[i++] != v2[j++]) {
     *			return false;
     *		    }
     *		}
     *		return true;
     *	    }
     *	}
     *	return false;
     *}
     */

    /**
     * Compares this <code>String</code> to another <code>String</code>,
     * ignoring case considerations.  Two strings are considered equal
     * ignoring case if they are of the same length, and corresponding
     * characters in the two strings are equal ignoring case.
     * <p>
     * Two characters <code>c1</code> and <code>c2</code> are considered
     * the same, ignoring case if at least one of the following is true:
     * <ul><li>The two characters are the same (as compared by the
     * <code>==</code> operator).
     * <li>Applying the method {@link java.lang.Character#toUpperCase(char)}
     * to each character produces the same result.
     * <li>Applying the method {@link java.lang.Character#toLowerCase(char)}
     * to each character produces the same result.</ul>
     *
     * @param   anotherString   the <code>String</code> to compare this
     *                          <code>String</code> against.
     * @return  <code>true</code> if the argument is not <code>null</code>
     *          and the <code>String</code>s are equal,
     *          ignoring case; <code>false</code> otherwise.
     * @see     #equals(Object)
     * @see     java.lang.Character#toLowerCase(char)
     * @see java.lang.Character#toUpperCase(char)
     */
    public boolean equalsIgnoreCase(String anotherString) {
        return (this == anotherString) ? true :
               (anotherString != null) && (anotherString.count == count) &&
	       regionMatches(true, 0, anotherString, 0, count);
    }

    /**
     * Compares two strings lexicographically.
     * The comparison is based on the Unicode value of each character in
     * the strings. The character sequence represented by this
     * <code>String</code> object is compared lexicographically to the
     * character sequence represented by the argument string. The result is
     * a negative integer if this <code>String</code> object
     * lexicographically precedes the argument string. The result is a
     * positive integer if this <code>String</code> object lexicographically
     * follows the argument string. The result is zero if the strings
     * are equal; <code>compareTo</code> returns <code>0</code> exactly when
     * the {@link #equals(Object)} method would return <code>true</code>.
     * <p>
     * This is the definition of lexicographic ordering. If two strings are
     * different, then either they have different characters at some index
     * that is a valid index for both strings, or their lengths are different,
     * or both. If they have different characters at one or more index
     * positions, let <i>k</i> be the smallest such index; then the string
     * whose character at position <i>k</i> has the smaller value, as
     * determined by using the &lt; operator, lexicographically precedes the
     * other string. In this case, <code>compareTo</code> returns the
     * difference of the two character values at position <code>k</code> in
     * the two string -- that is, the value:
     * <blockquote><pre>
     * this.charAt(k)-anotherString.charAt(k)
     * </pre></blockquote>
     * If there is no index position at which they differ, then the shorter
     * string lexicographically precedes the longer string. In this case,
     * <code>compareTo</code> returns the difference of the lengths of the
     * strings -- that is, the value:
     * <blockquote><pre>
     * this.length()-anotherString.length()
     * </pre></blockquote>
     *
     * @param   anotherString   the <code>String</code> to be compared.
     * @return  the value <code>0</code> if the argument string is equal to
     *          this string; a value less than <code>0</code> if this string
     *          is lexicographically less than the string argument; and a
     *          value greater than <code>0</code> if this string is
     *          lexicographically greater than the string argument.
     */
     public native int compareTo(String anotherString);

    /* 
     * The Java version of compare()
     *
     *public int compareTo(String anotherString) {
     *	int len1 = count;
     *	int len2 = anotherString.count;
     *	int n = Math.min(len1, len2);
     *	char v1[] = value;
     *	char v2[] = anotherString.value;
     *	int i = offset;
     *	int j = anotherString.offset;
     *
     *	if (i == j) {
     *	    int k = i;
     *	    int lim = n + i;
     *	    while (k < lim) {
     *		char c1 = v1[k];
     *		char c2 = v2[k];
     *		if (c1 != c2) {
     *		    return c1 - c2;
     *		}
     *		k++;
     *	    }
     *	} else {
     *	    while (n-- != 0) {
     *		char c1 = v1[i++];
     *		char c2 = v2[j++];
     *		if (c1 != c2) {
     *		    return c1 - c2;
     *		}
     *	    }
     *	}
     *	return len1 - len2;
     *}
     */

    /**
     * Compares this String to another Object.  If the Object is a String,
     * this function behaves like <code>compareTo(String)</code>.  Otherwise,
     * it throws a <code>ClassCastException</code> (as Strings are comparable
     * only to other Strings).
     *
     * @param   o the <code>Object</code> to be compared.
     * @return  the value <code>0</code> if the argument is a string
     *		lexicographically equal to this string; a value less than
     *		<code>0</code> if the argument is a string lexicographically
     *		greater than this string; and a value greater than
     *		<code>0</code> if the argument is a string lexicographically
     *		less than this string.
     * @exception <code>ClassCastException</code> if the argument is not a
     *		  <code>String</code>.
     * @see     java.lang.Comparable
     * @since   1.2
     */
    public int compareTo(Object o) {
	return compareTo((String)o);
    }

    /**
     * A Comparator that orders <code>String</code> objects as by
     * <code>compareToIgnoreCase</code>. This comparator is serializable.
     * <p>
     * Note that this Comparator does <em>not</em> take locale into account,
     * and will result in an unsatisfactory ordering for certain locales.
     * The java.text package provides <em>Collators</em> to allow
     * locale-sensitive ordering.
     *
     * @see     java.text.Collator#compare(String, String)
     * @since   1.2
     */
    public static final Comparator CASE_INSENSITIVE_ORDER
                                         = new CaseInsensitiveComparator();
    private static class CaseInsensitiveComparator
                         implements Comparator, java.io.Serializable {
	// use serialVersionUID from JDK 1.2.2 for interoperability
	private static final long serialVersionUID = 8575799808933029326L;

        public int compare(Object o1, Object o2) {
            String s1 = (String) o1;
            String s2 = (String) o2;
            int n1=s1.length(), n2=s2.length();
            for (int i1=0, i2=0; i1<n1 && i2<n2; i1++, i2++) {
                char c1 = s1.charAt(i1);
                char c2 = s2.charAt(i2);
                if (c1 != c2) {
                    c1 = Character.toUpperCase(c1);
                    c2 = Character.toUpperCase(c2);
                    if (c1 != c2) {
                        c1 = Character.toLowerCase(c1);
                        c2 = Character.toLowerCase(c2);
                        if (c1 != c2) {
                            return c1 - c2;
                        }
                    }
                }
            }
            return n1 - n2;
        }
    }

    /**
     * Compares two strings lexicographically, ignoring case
     * differences. This method returns an integer whose sign is that of
     * calling <code>compareTo</code> with normalized versions of the strings
     * where case differences have been eliminated by calling
     * <code>Character.toLowerCase(Character.toUpperCase(character))</code> on
     * each character.
     * <p>
     * Note that this method does <em>not</em> take locale into account,
     * and will result in an unsatisfactory ordering for certain locales.
     * The java.text package provides <em>collators</em> to allow
     * locale-sensitive ordering.
     *
     * @param   str   the <code>String</code> to be compared.
     * @return  a negative integer, zero, or a positive integer as the
     *		the specified String is greater than, equal to, or less
     *		than this String, ignoring case considerations.
     * @see     java.text.Collator#compare(String, String)
     * @since   1.2
     */
    public int compareToIgnoreCase(String str) {
        return CASE_INSENSITIVE_ORDER.compare(this, str);
    }

    /**
     * Tests if two string regions are equal.
     * <p>
     * A substring of this <tt>String</tt> object is compared to a substring
     * of the argument other. The result is true if these substrings
     * represent identical character sequences. The substring of this
     * <tt>String</tt> object to be compared begins at index <tt>toffset</tt>
     * and has length <tt>len</tt>. The substring of other to be compared
     * begins at index <tt>ooffset</tt> and has length <tt>len</tt>. The
     * result is <tt>false</tt> if and only if at least one of the following
     * is true:
     * <ul><li><tt>toffset</tt> is negative.
     * <li><tt>ooffset</tt> is negative.
     * <li><tt>toffset+len</tt> is greater than the length of this
     * <tt>String</tt> object.
     * <li><tt>ooffset+len</tt> is greater than the length of the other
     * argument.
     * <li>There is some nonnegative integer <i>k</i> less than <tt>len</tt>
     * such that:
     * <tt>this.charAt(toffset+<i>k</i>)&nbsp;!=&nbsp;other.charAt(ooffset+<i>k</i>)</tt>
     * </ul>
     *
     * @param   toffset   the starting offset of the subregion in this string.
     * @param   other     the string argument.
     * @param   ooffset   the starting offset of the subregion in the string
     *                    argument.
     * @param   len       the number of characters to compare.
     * @return  <code>true</code> if the specified subregion of this string
     *          exactly matches the specified subregion of the string argument;
     *          <code>false</code> otherwise.
     */
    public boolean regionMatches(int toffset, String other, int ooffset,
				 int len) {
	char ta[] = value;
	int to = offset + toffset;
	char pa[] = other.value;
	int po = other.offset + ooffset;
	// Note: toffset, ooffset, or len might be near -1>>>1.
	if ((ooffset < 0) || (toffset < 0) || (toffset > (long)count - len)
	    || (ooffset > (long)other.count - len)) {
	    return false;
	}
	while (len-- > 0) {
	    if (ta[to++] != pa[po++]) {
	        return false;
	    }
	}
	return true;
    }

    /**
     * Tests if two string regions are equal.
     * <p>
     * A substring of this <tt>String</tt> object is compared to a substring
     * of the argument <tt>other</tt>. The result is <tt>true</tt> if these
     * substrings represent character sequences that are the same, ignoring
     * case if and only if <tt>ignoreCase</tt> is true. The substring of
     * this <tt>String</tt> object to be compared begins at index
     * <tt>toffset</tt> and has length <tt>len</tt>. The substring of
     * <tt>other</tt> to be compared begins at index <tt>ooffset</tt> and
     * has length <tt>len</tt>. The result is <tt>false</tt> if and only if
     * at least one of the following is true:
     * <ul><li><tt>toffset</tt> is negative.
     * <li><tt>ooffset</tt> is negative.
     * <li><tt>toffset+len</tt> is greater than the length of this
     * <tt>String</tt> object.
     * <li><tt>ooffset+len</tt> is greater than the length of the other
     * argument.
     * <li><tt>ignoreCase</tt> is <tt>false</tt> and there is some nonnegative
     * integer <i>k</i> less than <tt>len</tt> such that:
     * <blockquote><pre>
     * this.charAt(toffset+k) != other.charAt(ooffset+k)
     * </pre></blockquote>
     * <li><tt>ignoreCase</tt> is <tt>true</tt> and there is some nonnegative
     * integer <i>k</i> less than <tt>len</tt> such that:
     * <blockquote><pre>
     * Character.toLowerCase(this.charAt(toffset+k)) !=
               Character.toLowerCase(other.charAt(ooffset+k))
     * </pre></blockquote>
     * and:
     * <blockquote><pre>
     * Character.toUpperCase(this.charAt(toffset+k)) !=
     *         Character.toUpperCase(other.charAt(ooffset+k))
     * </pre></blockquote>
     * </ul>
     *
     * @param   ignoreCase   if <code>true</code>, ignore case when comparing
     *                       characters.
     * @param   toffset      the starting offset of the subregion in this
     *                       string.
     * @param   other        the string argument.
     * @param   ooffset      the starting offset of the subregion in the string
     *                       argument.
     * @param   len          the number of characters to compare.
     * @return  <code>true</code> if the specified subregion of this string
     *          matches the specified subregion of the string argument;
     *          <code>false</code> otherwise. Whether the matching is exact
     *          or case insensitive depends on the <code>ignoreCase</code>
     *          argument.
     */
    public boolean regionMatches(boolean ignoreCase, int toffset,
                           String other, int ooffset, int len) {
        char ta[] = value;
        int to = offset + toffset;
        char pa[] = other.value;
        int po = other.offset + ooffset;
        // Note: toffset, ooffset, or len might be near -1>>>1.
        if ((ooffset < 0) || (toffset < 0) || (toffset > (long)count - len) ||
                (ooffset > (long)other.count - len)) {
            return false;
        }
        while (len-- > 0) {
            char c1 = ta[to++];
            char c2 = pa[po++];
            if (c1 == c2) {
                continue;
            }
            if (ignoreCase) {
                // If characters don't match but case may be ignored,
                // try converting both characters to uppercase.
                // If the results match, then the comparison scan should
                // continue.
                char u1 = Character.toUpperCase(c1);
                char u2 = Character.toUpperCase(c2);
                if (u1 == u2) {
                    continue;
                }
                // Unfortunately, conversion to uppercase does not work properly
                // for the Georgian alphabet, which has strange rules about case
                // conversion.  So we need to make one last check before
                // exiting.
                if (Character.toLowerCase(u1) == Character.toLowerCase(u2)) {
                    continue;
                }
            }
            return false;
        }
        return true;
    }

    /**
     * Tests if this string starts with the specified prefix beginning
     * a specified index.
     *
     * @param   prefix    the prefix.
     * @param   toffset   where to begin looking in the string.
     * @return  <code>true</code> if the character sequence represented by the
     *          argument is a prefix of the substring of this object starting
     *          at index <code>toffset</code>; <code>false</code> otherwise.
     *          The result is <code>false</code> if <code>toffset</code> is
     *          negative or greater than the length of this
     *          <code>String</code> object; otherwise the result is the same
     *          as the result of the expression
     *          <pre>
     *          this.subString(toffset).startsWith(prefix)
     *          </pre>
     */
    public boolean startsWith(String prefix, int toffset) {
	char ta[] = value;
	int to = offset + toffset;
	char pa[] = prefix.value;
	int po = prefix.offset;
	int pc = prefix.count;
	// Note: toffset might be near -1>>>1.
	if ((toffset < 0) || (toffset > count - pc)) {
	    return false;
	}
	while (--pc >= 0) {
	    if (ta[to++] != pa[po++]) {
	        return false;
	    }
	}
	return true;
    }

    /**
     * Tests if this string starts with the specified prefix.
     *
     * @param   prefix   the prefix.
     * @return  <code>true</code> if the character sequence represented by the
     *          argument is a prefix of the character sequence represented by
     *          this string; <code>false</code> otherwise.
     *          Note also that <code>true</code> will be returned if the
     *          argument is an empty string or is equal to this
     *          <code>String</code> object as determined by the
     *          {@link #equals(Object)} method.
     * @since   1. 0
     */
    public boolean startsWith(String prefix) {
	return startsWith(prefix, 0);
    }

    /**
     * Tests if this string ends with the specified suffix.
     *
     * @param   suffix   the suffix.
     * @return  <code>true</code> if the character sequence represented by the
     *          argument is a suffix of the character sequence represented by
     *          this object; <code>false</code> otherwise. Note that the
     *          result will be <code>true</code> if the argument is the
     *          empty string or is equal to this <code>String</code> object
     *          as determined by the {@link #equals(Object)} method.
     */
    public boolean endsWith(String suffix) {
	return startsWith(suffix, count - suffix.count);
    }

    /**
     * Returns a hash code for this string. The hash code for a
     * <code>String</code> object is computed as
     * <blockquote><pre>
     * s[0]*31^(n-1) + s[1]*31^(n-2) + ... + s[n-1]
     * </pre></blockquote>
     * using <code>int</code> arithmetic, where <code>s[i]</code> is the
     * <i>i</i>th character of the string, <code>n</code> is the length of
     * the string, and <code>^</code> indicates exponentiation.
     * (The hash value of the empty string is zero.)
     *
     * @return  a hash code value for this object.
     */
    public native int hashCode();

    /*
     * Original Java version
     *
     *public int hashCode() {
     *	int h = 0;
     *	int off = offset;
     *	char val[] = value;
     *	int len = count;
     *
     *	for (int i = 0; i < len; i++)
     *	    h = 31*h + val[off++];
     *
     *
     *	return h;
     *}
     */

    /**
     * Returns the index within this string of the first occurrence of the
     * specified character. If a character with value <code>ch</code> occurs
     * in the character sequence represented by this <code>String</code>
     * object, then the index of the first such occurrence is returned --
     * that is, the smallest value <i>k</i> such that:
     * <blockquote><pre>
     * this.charAt(<i>k</i>) == ch
     * </pre></blockquote>
     * is <code>true</code>. If no such character occurs in this string,
     * then <code>-1</code> is returned.
     *
     * @param   ch   a character.
     * @return  the index of the first occurrence of the character in the
     *          character sequence represented by this object, or
     *          <code>-1</code> if the character does not occur.
     */
    public native int indexOf(int ch);

    /*
     * The original Java version
     *
     *public int indexOf(int ch) {
     *	return indexOf(ch, 0);
     *}
     */

    /**
     * Code shared by String and StringBuffer to do searches. The
     * source is the character array being searched, and the target
     * is the string being searched for.
     *
     * @param   source       the characters being searched.
     * @param   sourceOffset offset of the source string.
     * @param   sourceCount  count of the source string.
     * @param   target       the characters being searched for.
     * @param   targetOffset offset of the target string.
     * @param   targetCount  count of the target string.
     * @param   fromIndex    the index to begin searching from.
     */
    static int indexOf(char[] source, int sourceOffset, int sourceCount,
                       char[] target, int targetOffset, int targetCount,
                       int fromIndex) {
        if (fromIndex >= sourceCount) {
            return (targetCount == 0 ? sourceCount : -1);
        }
        if (fromIndex < 0) {
            fromIndex = 0;
        }
        if (targetCount == 0) {
            return fromIndex;
        }

        char first  = target[targetOffset];
        int i = sourceOffset + fromIndex;
        int max = sourceOffset + (sourceCount - targetCount);

    startSearchForFirstChar:
        while (true) {
            /* Look for first character. */
            while (i <= max && source[i] != first) {
                i++;
            }
            if (i > max) {
                return -1;
            }

            /* Found first character, now look at the rest of v2 */
            int j = i + 1;
            int end = j + targetCount - 1;
            int k = targetOffset + 1;
            while (j < end) {
                if (source[j++] != target[k++]) {
                    i++;
                    /* Look for str's first char again. */
                    continue startSearchForFirstChar;
                }
            }
            return i - sourceOffset;    /* Found whole string. */
        }
    }

    /*
     * The original Java version
     *
     *public int indexOf(int ch) {
     *	return indexOf(ch, 0);
     *}
     */

    /**
     * Returns the index within this string of the first occurrence of the
     * specified character, starting the search at the specified index.
     * <p>
     * If a character with value <code>ch</code> occurs in the character
     * sequence represented by this <code>String</code> object at an index
     * no smaller than <code>fromIndex</code>, then the index of the first
     * such occurrence is returned--that is, the smallest value <i>k</i>
     * such that:
     * <blockquote><pre>
     * (this.charAt(<i>k</i>) == ch) && (<i>k</i> &gt;= fromIndex)
     * </pre></blockquote>
     * is true. If no such character occurs in this string at or after
     * position <code>fromIndex</code>, then <code>-1</code> is returned.
     * <p>
     * There is no restriction on the value of <code>fromIndex</code>. If it
     * is negative, it has the same effect as if it were zero: this entire
     * string may be searched. If it is greater than the length of this
     * string, it has the same effect as if it were equal to the length of
     * this string: <code>-1</code> is returned.
     *
     * @param   ch          a character.
     * @param   fromIndex   the index to start the search from.
     * @return  the index of the first occurrence of the character in the
     *          character sequence represented by this object that is greater
     *          than or equal to <code>fromIndex</code>, or <code>-1</code>
     *          if the character does not occur.
     */
    public native int indexOf(int ch, int fromIndex);

    /*
     * The original Java version
     *
     *public int indexOf(int ch, int fromIndex) {
     *	int max = offset + count;
     *	char v[] = value;
     *
     *	if (fromIndex < 0) {
     *	    fromIndex = 0;
     *	} else if (fromIndex >= count) {
     *	    // Note: fromIndex might be near -1>>>1.
     *	    return -1;
     *	}
     *	for (int i = offset + fromIndex ; i < max ; i++) {
     *	    if (v[i] == ch) {
     *		return i - offset;
     *	    }
     *	}
     *	return -1;
     *}
     */

    /**
     * Returns the index within this string of the last occurrence of the
     * specified character. That is, the index returned is the largest
     * value <i>k</i> such that:
     * <blockquote><pre>
     * this.charAt(<i>k</i>) == ch
     * </pre></blockquote>
     * is true.
     * The String is searched backwards starting at the last character.
     *
     * @param   ch   a character.
     * @return  the index of the last occurrence of the character in the
     *          character sequence represented by this object, or
     *          <code>-1</code> if the character does not occur.
     */
    public int lastIndexOf(int ch) {
     	return lastIndexOf(ch, count - 1);
    }

    /**
     * Returns the index within this string of the last occurrence of the
     * specified character, searching backward starting at the specified
     * index. That is, the index returned is the largest value <i>k</i>
     * such that:
     * <blockquote><pre>
     * this.charAt(k) == ch) && (k &lt;= fromIndex)
     * </pre></blockquote>
     * is true.
     *
     * @param   ch          a character.
     * @param   fromIndex   the index to start the search from. There is no
     *          restriction on the value of <code>fromIndex</code>. If it is
     *          greater than or equal to the length of this string, it has
     *          the same effect as if it were equal to one less than the
     *          length of this string: this entire string may be searched.
     *          If it is negative, it has the same effect as if it were -1:
     *          -1 is returned.
     * @return  the index of the last occurrence of the character in the
     *          character sequence represented by this object that is less
     *          than or equal to <code>fromIndex</code>, or <code>-1</code>
     *          if the character does not occur before that point.
     */
    public int lastIndexOf(int ch, int fromIndex) {
	int min = offset;
	char v[] = value;

	for (int i = offset + ((fromIndex >= count) ? count - 1 : fromIndex) ; i >= min ; i--) {
	    if (v[i] == ch) {
		return i - offset;
	    }
	}
	return -1;
    }

    /**
     * Returns the index within this string of the first occurrence of the
     * specified substring. The integer returned is the smallest value
     * <i>k</i> such that:
     * <blockquote><pre>
     * this.startsWith(str, <i>k</i>)
     * </pre></blockquote>
     * is <code>true</code>.
     *
     * @param   str   any string.
     * @return  if the string argument occurs as a substring within this
     *          object, then the index of the first character of the first
     *          such substring is returned; if it does not occur as a
     *          substring, <code>-1</code> is returned.
     */
    public native int indexOf(String str);

    /*
     * The original Java version
     *
     *public int indexOf(String str) {
     *	return indexOf(str, 0);
     *}
     */

    /**
     * Returns the index within this string of the first occurrence of the
     * specified substring, starting at the specified index.  The integer
     * returned is the smallest value <tt>k</tt> for which:
     * <blockquote><pre>
     *     k &gt;= Math.min(fromIndex, str.length()) && this.startsWith(str, k)
     * </pre></blockquote>
     * If no such value of <i>k</i> exists, then -1 is returned.
     *
     * @param   str         the substring for which to search.
     * @param   fromIndex   the index from which to start the search.
     * @return  the index within this string of the first occurrence of the
     *          specified substring, starting at the specified index.
     */
    public native int indexOf(String str, int fromIndex);

    /*
     * The original Java version
     *
     *public int indexOf(String str, int fromIndex) {
     *	char v1[] = value;
     *	char v2[] = str.value;
     *	int max = offset + (count - str.count);
     *	if (fromIndex >= count) {
     *	    if (count == 0 && fromIndex == 0 && str.count == 0) {
     *		/* There is an empty string at index 0 in an empty string.
     *		return 0;
     *	    }
     *	    /* Note: fromIndex might be near -1>>>1 
     *	    return -1;
     *	}
     *	if (fromIndex < 0) {
     *	    fromIndex = 0;
     *	}
     *	if (str.count == 0) {
     *	    return fromIndex;
     *	}
     *
     *	int strOffset = str.offset;
     *   char first  = v2[strOffset];
     *   int i = offset + fromIndex;
     *
     *startSearchForFirstChar:
     *   while (true) {
     *
     *	    /* Look for first character. 
     *	    while (i <= max && v1[i] != first) {
     *		i++;
     *	    }
     *	    if (i > max) {
     *		return -1;
     *	    }
     *
     *	    /* Found first character, now look at the rest of v2 
     *	    int j = i + 1;
     *	    int end = j + str.count - 1;
     *	    int k = strOffset + 1;
     *	    while (j < end) {
     *		if (v1[j++] != v2[k++]) {
     *		    i++;
     *		    /* Look for str's first char again. 
     *		    continue startSearchForFirstChar;
     *		}
     *	    }
     *	    return i - offset;	/* Found whole string. 
     *   }
     *}
     */

    /**
     * Returns the index within this string of the rightmost occurrence
     * of the specified substring.  The rightmost empty string "" is
     * considered to occur at the index value <code>this.length()</code>.
     * The returned index is the largest value <i>k</i> such that
     * <blockquote><pre>
     * this.startsWith(str, k)
     * </pre></blockquote>
     * is true.
     *
     * @param   str   the substring to search for.
     * @return  if the string argument occurs one or more times as a substring
     *          within this object, then the index of the first character of
     *          the last such substring is returned. If it does not occur as
     *          a substring, <code>-1</code> is returned.
     */
    public int lastIndexOf(String str) {
	return lastIndexOf(str, count);
    }

    /**
     * Returns the index within this string of the last occurrence of the
     * specified substring, searching backward starting at the specified index.
     * The integer returned is the largest value <i>k</i> such that:
     * <blockquote><pre>
     *     k &lt;= Math.min(fromIndex, str.length()) && this.startsWith(str, k)
     * </pre></blockquote>
     * If no such value of <i>k</i> exists, then -1 is returned.
     * 
     * @param   str         the substring to search for.
     * @param   fromIndex   the index to start the search from.
     * @return  the index within this string of the last occurrence of the
     *          specified substring.
     */
    public int lastIndexOf(String str, int fromIndex) {
        return lastIndexOf(value, offset, count,
                           str.value, str.offset, str.count, fromIndex);
    }

    /**
     * Code shared by String and StringBuffer to do searches. The
     * source is the character array being searched, and the target
     * is the string being searched for.
     *
     * @param   source       the characters being searched.
     * @param   sourceOffset offset of the source string.
     * @param   sourceCount  count of the source string.
     * @param   target       the characters being searched for.
     * @param   targetOffset offset of the target string.
     * @param   targetCount  count of the target string.
     * @param   fromIndex    the index to begin searching from.
     */
    static int lastIndexOf(char[] source, int sourceOffset, int sourceCount,
                           char[] target, int targetOffset, int targetCount,
                           int fromIndex) {
        /*
	 * Check arguments; return immediately where possible. For
	 * consistency, don't check for null str.
	 */
        int rightIndex = sourceCount - targetCount;
	if (fromIndex < 0) {
	    return -1;
	}
	if (fromIndex > rightIndex) {
	    fromIndex = rightIndex;
	}
	/* Empty string always matches. */
	if (targetCount == 0) {
	    return fromIndex;
	}

        int strLastIndex = targetOffset + targetCount - 1;
	char strLastChar = target[strLastIndex];
	int min = sourceOffset + targetCount - 1;
	int i = min + fromIndex;

    startSearchForLastChar:
	while (true) {
	    while (i >= min && source[i] != strLastChar) {
		i--;
	    }
	    if (i < min) {
		return -1;
	    }
	    int j = i - 1;
	    int start = j - (targetCount - 1);
	    int k = strLastIndex - 1;

	    while (j > start) {
	        if (source[j--] != target[k--]) {
		    i--;
		    continue startSearchForLastChar;
		}
	    }
	    return start - sourceOffset + 1;
	}
    }

    /**
     * Returns a new string that is a substring of this string. The
     * substring begins with the character at the specified index and
     * extends to the end of this string. <p>
     * Examples:
     * <blockquote><pre>
     * "unhappy".substring(2) returns "happy"
     * "Harbison".substring(3) returns "bison"
     * "emptiness".substring(9) returns "" (an empty string)
     * </pre></blockquote>
     *
     * @param      beginIndex   the beginning index, inclusive.
     * @return     the specified substring.
     * @exception  IndexOutOfBoundsException  if
     *             <code>beginIndex</code> is negative or larger than the
     *             length of this <code>String</code> object.
     */
    public String substring(int beginIndex) {
	return substring(beginIndex, count);
    }

    /**
     * Returns a new string that is a substring of this string. The
     * substring begins at the specified <code>beginIndex</code> and
     * extends to the character at index <code>endIndex - 1</code>.
     * Thus the length of the substring is <code>endIndex-beginIndex</code>.
     * <p>
     * Examples:
     * <blockquote><pre>
     * "hamburger".substring(4, 8) returns "urge"
     * "smiles".substring(1, 5) returns "mile"
     * </pre></blockquote>
     *
     * @param      beginIndex   the beginning index, inclusive.
     * @param      endIndex     the ending index, exclusive.
     * @return     the specified substring.
     * @exception  IndexOutOfBoundsException  if the
     *             <code>beginIndex</code> is negative, or
     *             <code>endIndex</code> is larger than the length of
     *             this <code>String</code> object, or
     *             <code>beginIndex</code> is larger than
     *             <code>endIndex</code>.
     */
    public String substring(int beginIndex, int endIndex) {
	if (beginIndex < 0) {
	    throw new StringIndexOutOfBoundsException(beginIndex);
	}
	if (endIndex > count) {
	    throw new StringIndexOutOfBoundsException(endIndex);
	}
	if (beginIndex > endIndex) {
	    throw new StringIndexOutOfBoundsException(endIndex - beginIndex);
	}
	return ((beginIndex == 0) && (endIndex == count)) ? this :
	    new String(offset + beginIndex, endIndex - beginIndex, value);
    }

    /**
     * Returns a new character sequence that is a subsequence of this sequence.
     *
     * <p> An invocation of this method of the form
     *
     * <blockquote><pre>
     * str.subSequence(begin,&nbsp;end)</pre></blockquote>
     *
     * behaves in exactly the same way as the invocation
     *
     * <blockquote><pre>
     * str.substring(begin,&nbsp;end)</pre></blockquote>
     *
     * This method is defined so that the <tt>String</tt> class can implement
     * the {@link CharSequence} interface. </p>
     *
     * @param      beginIndex   the begin index, inclusive.
     * @param      endIndex     the end index, exclusive.
     * @return     the specified subsequence.
     *
     * @throws  IndexOutOfBoundsException
     *          if <tt>beginIndex</tt> or <tt>endIndex</tt> are negative,
     *          if <tt>endIndex</tt> is greater than <tt>length()</tt>,
     *          or if <tt>beginIndex</tt> is greater than <tt>startIndex</tt>
     *
     * @since 1.4
     * @spec JSR-51
     */
    public CharSequence subSequence(int beginIndex, int endIndex) {
        return this.substring(beginIndex, endIndex);
    }

    /**
     * Concatenates the specified string to the end of this string.
     * <p>
     * If the length of the argument string is <code>0</code>, then this
     * <code>String</code> object is returned. Otherwise, a new
     * <code>String</code> object is created, representing a character
     * sequence that is the concatenation of the character sequence
     * represented by this <code>String</code> object and the character
     * sequence represented by the argument string.<p>
     * Examples:
     * <blockquote><pre>
     * "cares".concat("s") returns "caress"
     * "to".concat("get").concat("her") returns "together"
     * </pre></blockquote>
     *
     * @param   str   the <code>String</code> that is concatenated to the end
     *                of this <code>String</code>.
     * @return  a string that represents the concatenation of this object's
     *          characters followed by the string argument's characters.
     */
    public String concat(String str) {
	int otherLen = str.length();
	if (otherLen == 0) {
	    return this;
	}
	char buf[] = new char[count + otherLen];
	getChars(0, count, buf, 0);
	str.getChars(0, otherLen, buf, count);
	return new String(0, count + otherLen, buf);
    }

    /**
     * Returns a new string resulting from replacing all occurrences of
     * <code>oldChar</code> in this string with <code>newChar</code>.
     * <p>
     * If the character <code>oldChar</code> does not occur in the
     * character sequence represented by this <code>String</code> object,
     * then a reference to this <code>String</code> object is returned.
     * Otherwise, a new <code>String</code> object is created that
     * represents a character sequence identical to the character sequence
     * represented by this <code>String</code> object, except that every
     * occurrence of <code>oldChar</code> is replaced by an occurrence
     * of <code>newChar</code>.
     * <p>
     * Examples:
     * <blockquote><pre>
     * "mesquite in your cellar".replace('e', 'o')
     *         returns "mosquito in your collar"
     * "the war of baronets".replace('r', 'y')
     *         returns "the way of bayonets"
     * "sparring with a purple porpoise".replace('p', 't')
     *         returns "starring with a turtle tortoise"
     * "JonL".replace('q', 'x') returns "JonL" (no change)
     * </pre></blockquote>
     *
     * @param   oldChar   the old character.
     * @param   newChar   the new character.
     * @return  a string derived from this string by replacing every
     *          occurrence of <code>oldChar</code> with <code>newChar</code>.
     */
    public String replace(char oldChar, char newChar) {
	if (oldChar != newChar) {
	    int len = count;
	    int i = -1;
	    char[] val = value; /* avoid getfield opcode */
	    int off = offset;   /* avoid getfield opcode */

	    while (++i < len) {
		if (val[off + i] == oldChar) {
		    break;
		}
	    }
	    if (i < len) {
		char buf[] = new char[len];
		for (int j = 0 ; j < i ; j++) {
		    buf[j] = val[off+j];
		}
		while (i < len) {
		    char c = val[off + i];
		    buf[i] = (c == oldChar) ? newChar : c;
		    i++;
		}
		return new String(0, len, buf);
	    }
	}
	return this;
    }

    /**
     * Tells whether or not this string matches the given <a
     * href="../util/regex/Pattern.html#sum">regular expression</a>.
     *
     * <p> An invocation of this method of the form
     * <i>str</i><tt>.matches(</tt><i>regex</i><tt>)</tt> yields exactly the
     * same result as the expression
     *
     * <blockquote><tt> {@link java.util.regex.Pattern}.{@link
     * java.util.regex.Pattern#matches(String,CharSequence)
     * matches}(</tt><i>regex</i><tt>,</tt> <i>str</i><tt>)</tt></blockquote>
     *
     * @param   regex
     *          the regular expression to which this string is to be matched
     *
     * @return  <tt>true</tt> if, and only if, this string matches the
     *          given regular expression
     *
     * @throws  PatternSyntaxException
     *          if the regular expression's syntax is invalid
     *
     * @see java.util.regex.Pattern
     *
     * @since 1.4
     * @spec JSR-51
     */
    /*
    public boolean matches(String regex) {
        return Pattern.matches(regex, this);
    }
    */

    /**
     * Replaces the first substring of this string that matches the given <a
     * href="../util/regex/Pattern.html#sum">regular expression</a> with the
     * given replacement.
     *
     * <p> An invocation of this method of the form
     * <i>str</i><tt>.replaceFirst(</tt><i>regex</i><tt>,</tt> <i>repl</i><tt>)</tt>
     * yields exactly the same result as the expression
     *
     * <blockquote><tt>
     * {@link java.util.regex.Pattern}.{@link java.util.regex.Pattern#compile
     * compile}(</tt><i>regex</i><tt>).{@link
     * java.util.regex.Pattern#matcher(java.lang.CharSequence)
     * matcher}(</tt><i>str</i><tt>).{@link java.util.regex.Matcher#replaceFirst
     * replaceFirst}(</tt><i>repl</i><tt>)</tt></blockquote>
     *
     * @param   regex
     *          the regular expression to which this string is to be matched
     *
     * @return  The resulting <tt>String</tt>
     *
     * @throws  PatternSyntaxException
     *          if the regular expression's syntax is invalid
     *
     * @see java.util.regex.Pattern
     *
     * @since 1.4
     * @spec JSR-51
     */
    /*
    public String replaceFirst(String regex, String replacement) {
	return Pattern.compile(regex).matcher(this).replaceFirst(replacement);
    }
    */

    /**
     * Replaces each substring of this string that matches the given <a
     * href="../util/regex/Pattern.html#sum">regular expression</a> with the
     * given replacement.
     *
     * <p> An invocation of this method of the form
     * <i>str</i><tt>.replaceAll(</tt><i>regex</i><tt>,</tt> <i>repl</i><tt>)</tt>
     * yields exactly the same result as the expression
     *
     * <blockquote><tt>
     * {@link java.util.regex.Pattern}.{@link java.util.regex.Pattern#compile
     * compile}(</tt><i>regex</i><tt>).{@link
     * java.util.regex.Pattern#matcher(java.lang.CharSequence)
     * matcher}(</tt><i>str</i><tt>).{@link java.util.regex.Matcher#replaceAll
     * replaceAll}(</tt><i>repl</i><tt>)</tt></blockquote>
     *
     * @param   regex
     *          the regular expression to which this string is to be matched
     *
     * @return  The resulting <tt>String</tt>
     *
     * @throws  PatternSyntaxException
     *          if the regular expression's syntax is invalid
     *
     * @see java.util.regex.Pattern
     *
     * @since 1.4
     * @spec JSR-51
     */
    /*
    public String replaceAll(String regex, String replacement) {
	return Pattern.compile(regex).matcher(this).replaceAll(replacement);
    }
    */

    /**
     * Splits this string around matches of the given <a
     * href="{@docRoot}/java/util/regex/Pattern.html#sum">regular expression</a>.
     *
     * <p> The array returned by this method contains each substring of this
     * string that is terminated by another substring that matches the given
     * expression or is terminated by the end of the string.  The substrings in
     * the array are in the order in which they occur in this string.  If the
     * expression does not match any part of the input then the resulting array
     * has just one element, namely this string.
     *
     * <p> The <tt>limit</tt> parameter controls the number of times the
     * pattern is applied and therefore affects the length of the resulting
     * array.  If the limit <i>n</i> is greater than zero then the pattern
     * will be applied at most <i>n</i>&nbsp;-&nbsp;1 times, the array's
     * length will be no greater than <i>n</i>, and the array's last entry
     * will contain all input beyond the last matched delimiter.  If <i>n</i>
     * is non-positive then the pattern will be applied as many times as
     * possible and the array can have any length.  If <i>n</i> is zero then
     * the pattern will be applied as many times as possible, the array can
     * have any length, and trailing empty strings will be discarded.
     *
     * <p> The string <tt>"boo:and:foo"</tt>, for example, yields the
     * following results with these parameters:
     *
     * <blockquote><table cellpadding=1 cellspacing=0 summary="Split example showing regex, limit, and result">
     * <tr>
     *     <th>Regex</th>
     *     <th>Limit</th>
     *     <th>Result</th>
     * </tr>
     * <tr><td align=center>:</td>
     *     <td align=center>2</td>
     *     <td><tt>{ "boo", "and:foo" }</tt></td></tr>
     * <tr><td align=center>:</td>
     *     <td align=center>5</td>
     *     <td><tt>{ "boo", "and", "foo" }</tt></td></tr>
     * <tr><td align=center>:</td>
     *     <td align=center>-2</td>
     *     <td><tt>{ "boo", "and", "foo" }</tt></td></tr>
     * <tr><td align=center>o</td>
     *     <td align=center>5</td>
     *     <td><tt>{ "b", "", ":and:f", "", "" }</tt></td></tr>
     * <tr><td align=center>o</td>
     *     <td align=center>-2</td>
     *     <td><tt>{ "b", "", ":and:f", "", "" }</tt></td></tr>
     * <tr><td align=center>o</td>
     *     <td align=center>0</td>
     *     <td><tt>{ "b", "", ":and:f" }</tt></td></tr>
     * </table></blockquote>
     *
     * <p> An invocation of this method of the form
     * <i>str.</i><tt>split(</tt><i>regex</i><tt>,</tt>&nbsp;<i>n</i><tt>)</tt>
     * yields the same result as the expression
     *
     * <blockquote>
     * {@link java.util.regex.Pattern}.{@link java.util.regex.Pattern#compile
     * compile}<tt>(</tt><i>regex</i><tt>)</tt>.{@link
     * java.util.regex.Pattern#split(java.lang.CharSequence,int)
     * split}<tt>(</tt><i>str</i><tt>,</tt>&nbsp;<i>n</i><tt>)</tt>
     * </blockquote>
     *
     *
     * @param  regex
     *         the delimiting regular expression
     *
     * @param  limit
     *         the result threshold, as described above
     *
     * @return  the array of strings computed by splitting this string
     *          around matches of the given regular expression
     *
     * @throws  PatternSyntaxException
     *          if the regular expression's syntax is invalid
     *
     * @see java.util.regex.Pattern
     *
     * @since 1.4
     * @spec JSR-51
     */
    /*
    public String[] split(String regex, int limit) {
	return Pattern.compile(regex).split(this, limit);
    }
    */
  
    /**
     * Splits this string around matches of the given <a
     * href="{@docRoot}/java/util/regex/Pattern.html#sum">regular expression</a>.
     *
     * <p> This method works as if by invoking the two-argument {@link
     * #split(String, int) split} method with the given expression and a limit
     * argument of zero.  Trailing empty strings are therefore not included in
     * the resulting array.
     *
     * <p> The string <tt>"boo:and:foo"</tt>, for example, yields the following
     * results with these expressions:
     *
     * <blockquote><table cellpadding=1 cellspacing=0 summary="Split examples showing regex and result">
     * <tr>
     *  <th>Regex</th>
     *  <th>Result</th>
     * </tr>
     * <tr><td align=center>:</td>
     *     <td><tt>{ "boo", "and", "foo" }</tt></td></tr>
     * <tr><td align=center>o</td>
     *     <td><tt>{ "b", "", ":and:f" }</tt></td></tr>
     * </table></blockquote>
     *
     *
     * @param  regex
     *         the delimiting regular expression
     *
     * @return  the array of strings computed by splitting this string
     *          around matches of the given regular expression
     *
     * @throws  PatternSyntaxException
     *          if the regular expression's syntax is invalid
     *
     * @see java.util.regex.Pattern
     *
     * @since 1.4
     * @spec JSR-51
     */
    /*
    public String[] split(String regex) {
        return split(regex, 0);
    }
    */

    /**
     * Converts all of the characters in this <code>String</code> to lower
     * case using the rules of the given <code>Locale</code>.  Case mappings rely
     * heavily on the Unicode specification's character data. Since case
     * mappings are not always 1:1 char mappings, the resulting <code>String</code>
     * may be a different length than the original <code>String</code>.
     * <p>
     * Examples of lowercase  mappings are in the following table:
     * <table border="1" summary="Lowercase mapping examples showing language code of locale, upper case, lower case, and description">
     * <tr>
     *   <th>Language Code of Locale</th>
     *   <th>Upper Case</th>
     *   <th>Lower Case</th>
     *   <th>Description</th>
     * </tr>
     * <tr>
     *   <td>tr (Turkish)</td>
     *   <td>&#92;u0130</td>
     *   <td>&#92;u0069</td>
     *   <td>capital letter I with dot above -&gt; small letter i</td>
     * </tr>
     * <tr>
     *   <td>tr (Turkish)</td>
     *   <td>&#92;u0049</td>
     *   <td>&#92;u0131</td>
     *   <td>capital letter I -&gt; small letter dotless i </td>
     * </tr>
     * <tr>
     *   <td>(all)</td>
     *   <td>French Fries</td>
     *   <td>french fries</td>
     *   <td>lowercased all chars in String</td>
     * </tr>
     * <tr>
     *   <td>(all)</td>
     *   <td><img src="doc-files/capiota.gif" alt="capiota"><img src="doc-files/capchi.gif" alt="capchi">
     *       <img src="doc-files/captheta.gif" alt="captheta"><img src="doc-files/capupsil.gif" alt="capupsil">
     *       <img src="doc-files/capsigma.gif" alt="capsigma"></td>
     *   <td><img src="doc-files/iota.gif" alt="iota"><img src="doc-files/chi.gif" alt="chi">
     *       <img src="doc-files/theta.gif" alt="theta"><img src="doc-files/upsilon.gif" alt="upsilon">
     *       <img src="doc-files/sigma1.gif" alt="sigma"></td>
     *   <td>lowercased all chars in String</td>
     * </tr>
     * </table>
     *
     * @param locale use the case transformation rules for this locale
     * @return the <code>String</code>, converted to lowercase.
     * @see     java.lang.String#toLowerCase()
     * @see     java.lang.String#toUpperCase()
     * @see     java.lang.String#toUpperCase(Locale)
     * @since   1.1
     */
    public String toLowerCase(Locale locale) {
	if (locale == null)
	    throw new NullPointerException();
	
        int     len        = count;
        int     off        = offset;
        char[]  val        = value;
        int     firstUpper;

	/* Now check if there are any characters that need to be changed. */
	scan: {
	    for (firstUpper = 0 ; firstUpper < len ; firstUpper++) {
		char c = value[off+firstUpper];
		if (c != Character.toLowerCase(c)) {break scan;}
	    }
	    return this;
	}

        char[]  result = new char[count];

        /* Just copy the first few lowerCase characters. */
	CVM.copyCharArray(val, off, result, 0, firstUpper);
	
        if (locale.getLanguage().equals("tr")) {
            // special loop for Turkey
            for (int i = firstUpper; i < len; ++i) {
                char ch = val[off+i];
                if (ch == 'I') {
                    result[i] = '\u0131'; // dotless small i
                    continue;
                }
                if (ch == '\u0130') {       // dotted I
                    result[i] = 'i';      // dotted i
                    continue;
                }
                result[i] = Character.toLowerCase(ch);
            }
        } else {
            // normal, fast loop
            for (int i = firstUpper; i < len; ++i) {
                result[i] = Character.toLowerCase(val[off+i]);
            }
        }
	// Since we created the result array, just used the special
	// internal constructor to avoid cloning the char array:
	return new String(0, result.length, result);
    }

    /**
     * Converts all of the characters in this <code>String</code> to lower
     * case using the rules of the default locale. This is equivalent to calling
     * <code>toLowerCase(Locale.getDefault())</code>.
     * <p>
     * @return  the <code>String</code>, converted to lowercase.
     * @see     java.lang.String#toLowerCase(Locale)
     */
    public String toLowerCase() {
        return toLowerCase(Locale.getDefault());
    }

    /**
     * Converts all of the characters in this <code>String</code> to upper
     * case using the rules of the given <code>Locale</code>. Case mappings rely
     * heavily on the Unicode specification's character data. Since case mappings
     * are not always 1:1 char mappings, the resulting <code>String</code> may
     * be a different length than the original <code>String</code>.
     * <p>
     * Examples of locale-sensitive and 1:M case mappings are in the following table.
     * <p>
     * <table border="1" summary="Examples of locale-sensitive and 1:M case mappings. Shows Language code of locale, lower case, upper case, and description.">
     * <tr>
     *   <th>Language Code of Locale</th>
     *   <th>Lower Case</th>
     *   <th>Upper Case</th>
     *   <th>Description</th>
     * </tr>
     * <tr>
     *   <td>tr (Turkish)</td>
     *   <td>&#92;u0069</td>
     *   <td>&#92;u0130</td>
     *   <td>small letter i -&gt; capital letter I with dot above</td>
     * </tr>
     * <tr>
     *   <td>tr (Turkish)</td>
     *   <td>&#92;u0131</td>
     *   <td>&#92;u0049</td>
     *   <td>small letter dotless i -&gt; capital letter I</td>
     * </tr>
     * <tr>
     *   <td>(all)</td>
     *   <td>&#92;u00df</td>
     *   <td>&#92;u0053 &#92;u0053</td>
     *   <td>small letter sharp s -&gt; two letters: SS</td>
     * </tr>
     * <tr>
     *   <td>(all)</td>
     *   <td>Fahrvergn&uuml;gen</td>
     *   <td>FAHRVERGN&Uuml;GEN</td>
     *   <td></td>
     * </tr>
     * </table>
     * @param locale use the case transformation rules for this locale
     * @return the <code>String</code>, converted to uppercase.
     * @see     java.lang.String#toUpperCase()
     * @see     java.lang.String#toLowerCase()
     * @see     java.lang.String#toLowerCase(Locale)
     * @since   1.1
     */
    public String toUpperCase(Locale locale) {
        int     len        = count;
        int     off        = offset;
        char[]  val        = value;
        int     firstLower;

        /* Now check if there are any characters that need changing. */
        scan: {
            char upperCaseChar;
            char c;
            for (firstLower = 0 ; firstLower < len ; firstLower++) {
                c = value[off+firstLower];
                upperCaseChar = Character.toUpperCaseEx(c);
                if (upperCaseChar == Character.CHAR_ERROR || c != upperCaseChar) {
                    break scan;
                }
            }
            return this;
        }

        char[]  result       = new char[len]; /* might grow! */
	int     resultOffset = 0;  /* result grows, so i+resultOffset
				    * is the write location in result */

	/* Just copy the first few upperCase characters. */
	CVM.copyCharArray(val, off, result, 0, firstLower);

        if (locale.getLanguage().equals("tr")) {
            // special loop for Turkey
            char[] upperCharArray;
            char upperChar;
            char ch;

            for (int i = firstLower; i < len; ++i) {
                ch = val[off+i];
                if (ch == 'i') {
                    result[i+resultOffset] = '\u0130';  // dotted cap i
                    continue;
                }
                if (ch == '\u0131') {                   // dotless i
                    result[i+resultOffset] = 'I';       // cap I
                    continue;
                }
                upperChar = Character.toUpperCaseEx(ch);
                if (upperChar == Character.CHAR_ERROR) {
                    upperCharArray = Character.toUpperCaseCharArray(ch);
                    /* Grow result. */
                    int mapLen = upperCharArray.length;
                    char[] result2 = new char[result.length + mapLen - 1];
                    CVM.copyCharArray(result, 0, result2, 0,
                        i + 1 + resultOffset);
                    for (int x=0; x<mapLen; ++x) {
                        result2[i+resultOffset++] = upperCharArray[x];
                    }
                    --resultOffset;
                    result = result2;
                }
                else {
                    result[i+resultOffset] = upperChar;
                }
            }
        } else {
            // normal, fast loop
            char[] upperCharArray;
            char upperChar;
            char ch;
            for (int i = firstLower; i < len; ++i) {
                ch = val[off+i];
                upperChar = Character.toUpperCaseEx(ch);
                if (upperChar == Character.CHAR_ERROR) {
                    upperCharArray = Character.toUpperCaseCharArray(ch);
                    /* Grow result. */
                    int mapLen = upperCharArray.length;
                    char[] result2 = new char[result.length + mapLen - 1];
                    CVM.copyCharArray(result, 0, result2, 0,
                                              i + 1 + resultOffset);
                    for (int x=0; x<mapLen; ++x) {
                        result2[i+resultOffset++] = upperCharArray[x];
                    }
                    --resultOffset;
                    result = result2;
                }
                else {
                    result[i+resultOffset] = upperChar;
                }
            }
        }
	// Since we created the result array, just used the special
	// internal constructor to avoid cloning the char array:
	return new String(0, result.length, result);
    }

    /**
     * Converts all of the characters in this <code>String</code> to upper
     * case using the rules of the default locale. This method is equivalent to
     * <code>toUpperCase(Locale.getDefault())</code>.
     * <p>
     * @return  the <code>String</code>, converted to uppercase.
     * @see     java.lang.String#toUpperCase(Locale)
     */
    public String toUpperCase() {
        return toUpperCase(Locale.getDefault());
    }

    /**
     * Returns a copy of the string, with leading and trailing whitespace
     * omitted.
     * <p>
     * If this <code>String</code> object represents an empty character
     * sequence, or the first and last characters of character sequence
     * represented by this <code>String</code> object both have codes
     * greater than <code>'&#92;u0020'</code> (the space character), then a
     * reference to this <code>String</code> object is returned.
     * <p>
     * Otherwise, if there is no character with a code greater than
     * <code>'&#92;u0020'</code> in the string, then a new
     * <code>String</code> object representing an empty string is created
     * and returned.
     * <p>
     * Otherwise, let <i>k</i> be the index of the first character in the
     * string whose code is greater than <code>'&#92;u0020'</code>, and let
     * <i>m</i> be the index of the last character in the string whose code
     * is greater than <code>'&#92;u0020'</code>. A new <code>String</code>
     * object is created, representing the substring of this string that
     * begins with the character at index <i>k</i> and ends with the
     * character at index <i>m</i>-that is, the result of
     * <code>this.substring(<i>k</i>,&nbsp;<i>m</i>+1)</code>.
     * <p>
     * This method may be used to trim
     * {@link Character#isSpace(char) whitespace} from the beginning and end
     * of a string; in fact, it trims all ASCII control characters as well.
     *
     * @return  A copy of this string with leading and trailing white
     *          space removed, or this string if it has no leading or
     *          trailing white space.
     */
    public String trim() {
	int len = count;
	int st = 0;
	int off = offset;      /* avoid getfield opcode */
	char[] val = value;    /* avoid getfield opcode */

	while ((st < len) && (val[off + st] <= ' ')) {
	    st++;
	}
	while ((st < len) && (val[off + len - 1] <= ' ')) {
	    len--;
	}
	return ((st > 0) || (len < count)) ? substring(st, len) : this;
    }

    /**
     * This object (which is already a string!) is itself returned.
     *
     * @return  the string itself.
     */
    public String toString() {
	return this;
    }

    /**
     * Converts this string to a new character array.
     *
     * @return  a newly allocated character array whose length is the length
     *          of this string and whose contents are initialized to contain
     *          the character sequence represented by this string.
     */
    public char[] toCharArray() {
	char result[] = new char[count];
	getChars(0, count, result, 0);
	return result;
    }

    /**
     * Returns the string representation of the <code>Object</code> argument.
     *
     * @param   obj   an <code>Object</code>.
     * @return  if the argument is <code>null</code>, then a string equal to
     *          <code>"null"</code>; otherwise, the value of
     *          <code>obj.toString()</code> is returned.
     * @see     java.lang.Object#toString()
     */
    public static String valueOf(Object obj) {
	return (obj == null) ? "null" : obj.toString();
    }

    /**
     * Returns the string representation of the <code>char</code> array
     * argument. The contents of the character array are copied; subsequent
     * modification of the character array does not affect the newly
     * created string.
     *
     * @param   data   a <code>char</code> array.
     * @return  a newly allocated string representing the same sequence of
     *          characters contained in the character array argument.
     */
    public static String valueOf(char data[]) {
	return new String(data);
    }

    /**
     * Returns the string representation of a specific subarray of the
     * <code>char</code> array argument.
     * <p>
     * The <code>offset</code> argument is the index of the first
     * character of the subarray. The <code>count</code> argument
     * specifies the length of the subarray. The contents of the subarray
     * are copied; subsequent modification of the character array does not
     * affect the newly created string.
     *
     * @param   data     the character array.
     * @param   offset   the initial offset into the value of the
     *                  <code>String</code>.
     * @param   count    the length of the value of the <code>String</code>.
     * @return  a string representing the sequence of characters contained 
     *          in the subarray of the character array argument.
     * @exception IndexOutOfBoundsException if <code>offset</code> is
     *          negative, or <code>count</code> is negative, or
     *          <code>offset+count</code> is larger than
     *          <code>data.length</code>.
     */
    public static String valueOf(char data[], int offset, int count) {
	return new String(data, offset, count);
    }

    /**
     * Returns a String that represents the character sequence in the
     * array specified.
     *
     * @param   data     the character array.
     * @param   offset   initial offset of the subarray.
     * @param   count    length of the subarray.
     * @return  a <code>String</code> that contains the characters of the
     *          specified subarray of the character array.
     */
    public static String copyValueOf(char data[], int offset, int count) {
	// All public String constructors now copy the data.
	return new String(data, offset, count);
    }

    /**
     * Returns a String that represents the character sequence in the
     * array specified.
     *
     * @param   data   the character array.
     * @return  a <code>String</code> that contains the characters of the
     *          character array.
     */
    public static String copyValueOf(char data[]) {
	return copyValueOf(data, 0, data.length);
    }

    /**
     * Returns the string representation of the <code>boolean</code> argument.
     *
     * @param   b   a <code>boolean</code>.
     * @return  if the argument is <code>true</code>, a string equal to
     *          <code>"true"</code> is returned; otherwise, a string equal to
     *          <code>"false"</code> is returned.
     */
    public static String valueOf(boolean b) {
	return b ? "true" : "false";
    }

    /**
     * Returns the string representation of the <code>char</code>
     * argument.
     *
     * @param   c   a <code>char</code>.
     * @return  a string of length <code>1</code> containing
     *          as its single character the argument <code>c</code>.
     */
    public static String valueOf(char c) {
	char data[] = {c};
	return new String(0, 1, data);
    }

    /**
     * Returns the string representation of the <code>int</code> argument.
     * <p>
     * The representation is exactly the one returned by the
     * <code>Integer.toString</code> method of one argument.
     *
     * @param   i   an <code>int</code>.
     * @return  a string representation of the <code>int</code> argument.
     * @see     java.lang.Integer#toString(int, int)
     */
    public static String valueOf(int i) {
        return Integer.toString(i, 10);
    }

    /**
     * Returns the string representation of the <code>long</code> argument.
     * <p>
     * The representation is exactly the one returned by the
     * <code>Long.toString</code> method of one argument.
     *
     * @param   l   a <code>long</code>.
     * @return  a string representation of the <code>long</code> argument.
     * @see     java.lang.Long#toString(long)
     */
    public static String valueOf(long l) {
        return Long.toString(l, 10);
    }

    /**
     * Returns the string representation of the <code>float</code> argument.
     * <p>
     * The representation is exactly the one returned by the
     * <code>Float.toString</code> method of one argument.
     *
     * @param   f   a <code>float</code>.
     * @return  a string representation of the <code>float</code> argument.
     * @see     java.lang.Float#toString(float)
     */
    public static String valueOf(float f) {
	return Float.toString(f);
    }

    /**
     * Returns the string representation of the <code>double</code> argument.
     * <p>
     * The representation is exactly the one returned by the
     * <code>Double.toString</code> method of one argument.
     *
     * @param   d   a <code>double</code>.
     * @return  a  string representation of the <code>double</code> argument.
     * @see     java.lang.Double#toString(double)
     */
    public static String valueOf(double d) {
	return Double.toString(d);
    }

    /**
     * Returns a canonical representation for the string object.
     * <p>
     * A pool of strings, initially empty, is maintained privately by the
     * class <code>String</code>.
     * <p>
     * When the intern method is invoked, if the pool already contains a
     * string equal to this <code>String</code> object as determined by
     * the {@link #equals(Object)} method, then the string from the pool is
     * returned. Otherwise, this <code>String</code> object is added to the
     * pool and a reference to this <code>String</code> object is returned.
     * <p>
     * It follows that for any two strings <code>s</code> and <code>t</code>,
     * <code>s.intern()&nbsp;==&nbsp;t.intern()</code> is <code>true</code>
     * if and only if <code>s.equals(t)</code> is <code>true</code>.
     * <p>
     * All literal strings and string-valued constant expressions are
     * interned. String literals are defined in &sect;3.10.5 of the
     * <a href="http://java.sun.com/docs/books/jls/html/">Java Language
     * Specification</a>
     *
     * @return  a string that has the same contents as this string, but is
     *          guaranteed to be from a pool of unique strings.
     */

    public native String intern();
}