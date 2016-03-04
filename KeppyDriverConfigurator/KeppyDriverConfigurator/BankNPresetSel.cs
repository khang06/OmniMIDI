﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace KeppyDriverConfigurator
{
    public partial class BankNPresetSel : Form
    {
        public string BankValueReturn { get; set; }
        public string PresetValueReturn { get; set; }

        public BankNPresetSel()
        {
            InitializeComponent();

        }

        private void PresetSel_Load(object sender, EventArgs e)
        {
            BankVal.Value = 0;
            PresetVal.Value = 0;
        }

        private void ConfirmBut_Click(object sender, EventArgs e)
        {
            BankValueReturn = BankVal.Value.ToString();
            PresetValueReturn = PresetVal.Value.ToString();
            DialogResult = DialogResult.OK;
            Close();
        }
    }
}
