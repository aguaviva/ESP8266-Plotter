//////////////////////////////////////////////////////////////////////
class myPath
{
     /**
       * Constructor
       * @param {Array|String} path Path data (sequence of coordinates and corresponding "command" tokens)
       * @param {Object} [options] Options object
       * @return {fabric.Path} thisArg
       */
      constructor(path)
      {
        this.commandLengths = {
          m: 2,
          l: 2,
          h: 1,
          v: 1,
          c: 6,
          s: 4,
          q: 4,
          t: 2,
          a: 7
        };

        this.commandLengths = {
          m: 2,
          l: 2,
          h: 1,
          v: 1,
          c: 6,
          s: 4,
          q: 4,
          t: 2,
          a: 7
        };

        this.repeatedCommands = {
          m: 'l',
          M: 'L'
        };

        /**
         * Array of path points
         * @type Array
         * @default
         */
        this.path = null;

        if (!path) {
          path = [];
        }

        var fromArray = Object.prototype.toString.call(path) === '[object Array]';

        this.path = fromArray
          ? path
          // one of commands (m,M,l,L,q,Q,c,C,etc.) followed by non-command characters (i.e. command values)
          : path.match && path.match(/[mzlhvcsqta][^mzlhvcsqta]*/gi);

        if (!this.path) {
          return;
        }

        if (!fromArray) {
          this.path = this.parsePath();
        }

        //this._setPositionDimensions(options);
      }

      /**
       * @private
       */
      parsePath() {
        var result = [],
            coords = [],
            currentPath,
            parsed,
            re = /([-+]?((\d+\.\d+)|((\d+)|(\.\d+)))(?:e[-+]?\d+)?)/ig,
            match,
            coordsStr;

        for (var i = 0, coordsParsed, len = this.path.length; i < len; i++) {
          currentPath = this.path[i];

          coordsStr = currentPath.slice(1).trim();
          coords.length = 0;

          while ((match = re.exec(coordsStr))) {
            coords.push(match[0]);
          }

          coordsParsed = [currentPath.charAt(0)];

          for (var j = 0, jlen = coords.length; j < jlen; j++) {
            parsed = parseFloat(coords[j]);
            if (!isNaN(parsed)) {
              coordsParsed.push(parsed);
            }
          }

          var command = coordsParsed[0],
              commandLength = this.commandLengths[command.toLowerCase()],
              repeatedCommand = this.repeatedCommands[command] || command;

          if (coordsParsed.length - 1 > commandLength) {
            for (var k = 1, klen = coordsParsed.length; k < klen; k += commandLength) {
              result.push([command].concat(coordsParsed.slice(k, k + commandLength)));
              command = repeatedCommand;
            }
          }
          else {
            result.push(coordsParsed);
          }
        }

        return result;
      }

     /**
     * @private
     * @param {CanvasRenderingContext2D} ctx context to render path on
     */
    renderPathCommands(ctx) {
      var current, // current instruction
          previous = null,
          subpathStartX = 0,
          subpathStartY = 0,
          x = 0, // current x
          y = 0, // current y
          controlX = 0, // current control point x
          controlY = 0, // current control point y
          tempX,
          tempY,
          l = 0,//-this.pathOffset.x,
          t = 0;//-this.pathOffset.y;

      ctx.beginPath();

      for (var i = 0, len = this.path.length; i < len; ++i) {

        current = this.path[i];

        switch (current[0]) { // first letter

          case 'l': // lineto, relative
            x += current[1];
            y += current[2];
            ctx.lineTo(x + l, y + t);
            break;

          case 'L': // lineto, absolute
            x = current[1];
            y = current[2];
            ctx.lineTo(x + l, y + t);
            break;

          case 'h': // horizontal lineto, relative
            x += current[1];
            ctx.lineTo(x + l, y + t);
            break;

          case 'H': // horizontal lineto, absolute
            x = current[1];
            ctx.lineTo(x + l, y + t);
            break;

          case 'v': // vertical lineto, relative
            y += current[1];
            ctx.lineTo(x + l, y + t);
            break;

          case 'V': // verical lineto, absolute
            y = current[1];
            ctx.lineTo(x + l, y + t);
            break;

          case 'm': // moveTo, relative
            x += current[1];
            y += current[2];
            subpathStartX = x;
            subpathStartY = y;
            ctx.moveTo(x + l, y + t);
            break;

          case 'M': // moveTo, absolute
            x = current[1];
            y = current[2];
            subpathStartX = x;
            subpathStartY = y;
            ctx.moveTo(x + l, y + t);
            break;

          case 'c': // bezierCurveTo, relative
            tempX = x + current[5];
            tempY = y + current[6];
            controlX = x + current[3];
            controlY = y + current[4];
            ctx.bezierCurveTo(
              x + current[1] + l, // x1
              y + current[2] + t, // y1
              controlX + l, // x2
              controlY + t, // y2
              tempX + l,
              tempY + t
            );
            x = tempX;
            y = tempY;
            break;

          case 'C': // bezierCurveTo, absolute
            x = current[5];
            y = current[6];
            controlX = current[3];
            controlY = current[4];
            ctx.bezierCurveTo(
              current[1] + l,
              current[2] + t,
              controlX + l,
              controlY + t,
              x + l,
              y + t
            );
            break;

          case 's': // shorthand cubic bezierCurveTo, relative

            // transform to absolute x,y
            tempX = x + current[3];
            tempY = y + current[4];

            if (previous[0].match(/[CcSs]/) === null) {
              // If there is no previous command or if the previous command was not a C, c, S, or s,
              // the control point is coincident with the current point
              controlX = x;
              controlY = y;
            }
            else {
              // calculate reflection of previous control points
              controlX = 2 * x - controlX;
              controlY = 2 * y - controlY;
            }

            ctx.bezierCurveTo(
              controlX + l,
              controlY + t,
              x + current[1] + l,
              y + current[2] + t,
              tempX + l,
              tempY + t
            );
            // set control point to 2nd one of this command
            // "... the first control point is assumed to be
            // the reflection of the second control point on
            // the previous command relative to the current point."
            controlX = x + current[1];
            controlY = y + current[2];

            x = tempX;
            y = tempY;
            break;

          case 'S': // shorthand cubic bezierCurveTo, absolute
            tempX = current[3];
            tempY = current[4];
            if (previous[0].match(/[CcSs]/) === null) {
              // If there is no previous command or if the previous command was not a C, c, S, or s,
              // the control point is coincident with the current point
              controlX = x;
              controlY = y;
            }
            else {
              // calculate reflection of previous control points
              controlX = 2 * x - controlX;
              controlY = 2 * y - controlY;
            }
            ctx.bezierCurveTo(
              controlX + l,
              controlY + t,
              current[1] + l,
              current[2] + t,
              tempX + l,
              tempY + t
            );
            x = tempX;
            y = tempY;

            // set control point to 2nd one of this command
            // "... the first control point is assumed to be
            // the reflection of the second control point on
            // the previous command relative to the current point."
            controlX = current[1];
            controlY = current[2];

            break;

          case 'q': // quadraticCurveTo, relative
            // transform to absolute x,y
            tempX = x + current[3];
            tempY = y + current[4];

            controlX = x + current[1];
            controlY = y + current[2];

            ctx.quadraticCurveTo(
              controlX + l,
              controlY + t,
              tempX + l,
              tempY + t
            );
            x = tempX;
            y = tempY;
            break;

          case 'Q': // quadraticCurveTo, absolute
            tempX = current[3];
            tempY = current[4];

            ctx.quadraticCurveTo(
              current[1] + l,
              current[2] + t,
              tempX + l,
              tempY + t
            );
            x = tempX;
            y = tempY;
            controlX = current[1];
            controlY = current[2];
            break;

          case 't': // shorthand quadraticCurveTo, relative

            // transform to absolute x,y
            tempX = x + current[1];
            tempY = y + current[2];

            if (previous[0].match(/[QqTt]/) === null) {
              // If there is no previous command or if the previous command was not a Q, q, T or t,
              // assume the control point is coincident with the current point
              controlX = x;
              controlY = y;
            }
            else {
              // calculate reflection of previous control point
              controlX = 2 * x - controlX;
              controlY = 2 * y - controlY;
            }

            ctx.quadraticCurveTo(
              controlX + l,
              controlY + t,
              tempX + l,
              tempY + t
            );
            x = tempX;
            y = tempY;

            break;

          case 'T':
            tempX = current[1];
            tempY = current[2];

            if (previous[0].match(/[QqTt]/) === null) {
              // If there is no previous command or if the previous command was not a Q, q, T or t,
              // assume the control point is coincident with the current point
              controlX = x;
              controlY = y;
            }
            else {
              // calculate reflection of previous control point
              controlX = 2 * x - controlX;
              controlY = 2 * y - controlY;
            }
            ctx.quadraticCurveTo(
              controlX + l,
              controlY + t,
              tempX + l,
              tempY + t
            );
            x = tempX;
            y = tempY;
            break;

          case 'a':
            // TODO: optimize this
            drawArc(ctx, x + l, y + t, [
              current[1],
              current[2],
              current[3],
              current[4],
              current[5],
              current[6] + x + l,
              current[7] + y + t
            ]);
            x += current[6];
            y += current[7];
            break;

          case 'A':
            // TODO: optimize this
            drawArc(ctx, x + l, y + t, [
              current[1],
              current[2],
              current[3],
              current[4],
              current[5],
              current[6] + l,
              current[7] + t
            ]);
            x = current[6];
            y = current[7];
            break;

          case 'z':
          case 'Z':
            x = subpathStartX;
            y = subpathStartY;
            ctx.closePath();
            break;
        }
        previous = current;
      }

      ctx.stroke();
    }
}

//////////////////////////////////////////////////////////////////////

class SpecialContext
{
  constructor(scale)
  {
    this.path = [];
    this.scale = scale;
    //this.addPoint(0,0);
  }

  addPoint(x,y)
  {
    this.path.push(x*this.scale);
    this.path.push(y*this.scale);

    this.x0 = x
    this.y0 = y
  }

  beginPath()
  {
  }

  moveTo(x,y)
  {
    this.addPoint(x,y);
  }

  lineTo(x,y)
  {
    this.addPoint(x,y);
  }

  quadraticCurveTo(x1,y1,x2,y2)
  {
    for (var step = 0; step<=10;step++)
    {
      var t = step/10.0;

      var x = (1-t)*(1-t)*this.x0 + 2*(1-t)*t*x1 + t*t*x2;
      var y = (1-t)*(1-t)*this.y0 + 2*(1-t)*t*y1 + t*t*y2;

      this.addPoint(x,y);
    }
  }

  bezierCurveTo(x1,y1,x2,y2,x3,y3)
  {
    for (var step = 0; step<=5;step++)
    {
      var t = step/5.0;

      var x = (1-t)*(1-t)*(1-t)*this.x0 + 3*(1-t)*(1-t)*t*x1 + 3*(1-t)*t*t*x2 + t*t*t*x3
      var y = (1-t)*(1-t)*(1-t)*this.y0 + 3*(1-t)*(1-t)*t*y1 + 3*(1-t)*t*t*y2 + t*t*t*y3

      this.addPoint(x,y);
    }
  }

  closePath()
  {
    //Print("closePath\n");
  }

  stroke()
  {
    //Print("stroke\n");
  }

  getPath()
  {
    return this.path;
  }
}
