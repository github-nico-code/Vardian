---
layout: default
---

This software is still in alpha status.

Vardian is a program to use the Rokid Max device to look at a multi monitor setup 
for Windows PCs. It uses the 3DOF functionality to navigate
a Window opened at the Rokid Max screen about the virtual windows
monitors.

# Releases

Releases can be found at [Vardian Github Releases Page](https://github.com/github-nico-code/Vardian/releases)

# Add virtual Screen to Windows

To add more virtual monitors to my setup without real monitors and
without Admin access I take a HDMI dummy plug like 
[One Dummy HDMI example device](https://www.amazon.de/dp/B07YMTKJCR?ref=ppx_yo2ov_dt_b_product_details&amp;th=1).

An alternative approach can be virtual monitor drivers like
[IddSampleDriver](https://github.com/roshkins/IddSampleDriver).

# Used Code Sources

I have taken some files from [Monado](https://monado.pages.freedesktop.org/monado/index.html)
for this project. Thanks for that.

I have taken some files from microsoft samples. Thanks for that.

# Software functionality

What does this software do:
1. Detect whether Rokid Max is connected.
1. Move Rokid Max screen to the most right in Windows Virtual screen
   setup.
1. Place a window at the Rokid Max screen.
1. Copy Windows screen regions to the window at the Rokid Max screen.
1. Move the copied screen region with the help of Rokid Max 3DOF
        functionality.

# TODOs

This program has some open TODOs. If you want to take over project or do
      some changes then please write me: nico.kasprzyk@gmx.de

Next steps could be:
* Implement a lot of comfort functionality
* Stability
* Use additional HID commands to get and set Rokid Max screen mode
* 

# Web resources

[Void Computing Rokid Max information](https://voidcomputing.hu/blog/good-bad-ugly/#the-rokid-air-mcu-protocol)


Text can be **bold**, _italic_, or ~~strikethrough~~.

[Link to another page](./another-page.html).

There should be whitespace between paragraphs.

There should be whitespace between paragraphs. We recommend including a README, or a file with information about your project.

# Header 1

This is a normal paragraph following a header. GitHub is a code hosting platform for version control and collaboration. It lets you and others work together on projects from anywhere.

## Header 2

> This is a blockquote following a header.
>
> When something is important enough, you do it even if the odds are not in your favor.

### Header 3

```js
// Javascript code with syntax highlighting.
var fun = function lang(l) {
  dateformat.i18n = require('./lang/' + l)
  return true;
}
```

```ruby
# Ruby code with syntax highlighting
GitHubPages::Dependencies.gems.each do |gem, version|
  s.add_dependency(gem, "= #{version}")
end
```

#### Header 4

*   This is an unordered list following a header.
*   This is an unordered list following a header.
*   This is an unordered list following a header.

##### Header 5

1.  This is an ordered list following a header.
2.  This is an ordered list following a header.
3.  This is an ordered list following a header.

###### Header 6

| head1        | head two          | three |
|:-------------|:------------------|:------|
| ok           | good swedish fish | nice  |
| out of stock | good and plenty   | nice  |
| ok           | good `oreos`      | hmm   |
| ok           | good `zoute` drop | yumm  |

### There's a horizontal rule below this.

* * *

### Here is an unordered list:

*   Item foo
*   Item bar
*   Item baz
*   Item zip

### And an ordered list:

1.  Item one
1.  Item two
1.  Item three
1.  Item four

### And a nested list:

- level 1 item
  - level 2 item
  - level 2 item
    - level 3 item
    - level 3 item
- level 1 item
  - level 2 item
  - level 2 item
  - level 2 item
- level 1 item
  - level 2 item
  - level 2 item
- level 1 item

### Small image

![Octocat](https://github.githubassets.com/images/icons/emoji/octocat.png)

### Large image

![Branching](https://guides.github.com/activities/hello-world/branching.png)


### Definition lists can be used with HTML syntax.

<dl>
<dt>Name</dt>
<dd>Godzilla</dd>
<dt>Born</dt>
<dd>1952</dd>
<dt>Birthplace</dt>
<dd>Japan</dd>
<dt>Color</dt>
<dd>Green</dd>
</dl>

```
Long, single-line code blocks should not wrap. They should horizontally scroll if they are too long. This line should be long enough to demonstrate this.
```

```
The final element.
```
