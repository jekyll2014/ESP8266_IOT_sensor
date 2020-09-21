using System.Collections.Generic;

namespace ChartPlotMQTT
{
    struct MinMaxValue
    {
        public float MaxValue;
        public float MinValue;
    }

    class PlotItems
    {
        private readonly List<string> plots = new List<string>();
        private readonly List<MinMaxValue> valueLimits = new List<MinMaxValue>();
        private readonly List<bool> active = new List<bool>();

        public bool Add(string newPlot, bool activePlot = false)
        {
            if (plots.Contains(newPlot)) return false;

            plots.Add(newPlot);
            valueLimits.Add(new MinMaxValue());
            active.Add(activePlot);
            return true;
        }

        public bool Remove(string item)
        {
            var itemNum = ItemNumber(item);
            if (itemNum < 0) return false;

            plots.RemoveAt(itemNum);
            valueLimits.RemoveAt(itemNum);
            active.RemoveAt(itemNum);
            return true;
        }

        public bool Remove(int item)
        {
            if (item >= plots.Count) return false;

            plots.RemoveAt(item);
            valueLimits.RemoveAt(item);
            active.RemoveAt(item);
            return true;
        }

        public void Clear()
        {
            plots.Clear();
            valueLimits.Clear();
            active.Clear();
        }

        public bool Contains(string plot)
        {
            return plots.Contains(plot);
        }

        // The get accessor returns a newValue for a given string
        public MinMaxValue GetValueRange(string plotName)
        {
            return valueLimits[ItemNumber(plotName)];
        }

        public MinMaxValue GetValueRange(int plotNumber)
        {
            return valueLimits[plotNumber];
        }

        public void SetValueRange(string plotName, MinMaxValue newValue)
        {
            valueLimits[ItemNumber(plotName)] = newValue;
        }

        public void SetValueRange(int plotNumber, MinMaxValue newValue)
        {
            valueLimits[plotNumber] = newValue;
        }

        public int ItemNumber(string item)
        {
            if (!plots.Contains(item)) return -1;

            return plots.FindIndex(x => x == item);
        }

        public string ItemName(int plotNumber)
        {
            return plots[plotNumber];
        }

        public bool SetActive(string item, bool itemActive)
        {
            var itemNumber = ItemNumber(item);
            if (itemNumber < 0) return false;

            active[itemNumber] = itemActive;

            return true;
        }

        public bool GetActive(string item)
        {
            var itemNumber = ItemNumber(item);
            return itemNumber >= 0 && active[ItemNumber(item)];
        }

        public int Count => plots.Count;
    }
}
