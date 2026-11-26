// day.js — a day as the studio writes it: "6 Nov", "6 Nov 2026" (this computer's day; the months in English, always
// three letters). update.js (a release's day) and board.js (when the board was loaded) read it here. start.js's "when"
// is its own on purpose: Now, 5 min ago, Yesterday — and en-GB's months ("Sept").
const MON = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
export const day = (d, year = false) => {
  const t = new Date(d || '');
  return isNaN(t) ? '' : `${t.getDate()} ${MON[t.getMonth()]}${year ? ' ' + t.getFullYear() : ''}`;
};
